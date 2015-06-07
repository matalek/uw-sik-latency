#include "shared.h"
#include "mdns_client.h"
#include "computer.h"
#include "mdns_fields.h"
#include "name_server.hpp"
#include "mdns_server.h"

using namespace std;


mdns_server* mdns_server_;
mdns_unicast_server* mdns_unicast_server_;

mdns_server::mdns_server() {
	start_receive();
}

void mdns_server::announce_name() {
	send_response(dns_type::A, service::UDP, false, false);
	if (announce_ssh_service)
		send_response(dns_type::A, service::TCP, false, false);
}

void mdns_server::start_receive() {
	deb(cout << "czekam...\n";)
	socket_mdns->async_receive_from(
		boost::asio::buffer(recv_buffer_), remote_endpoint_,
		boost::bind(&mdns_server::handle_receive, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void mdns_server::handle_receive(const boost::system::error_code& error,
  std::size_t size /*bytes_transferred*/) {
	if (error || size > BUFFER_SIZE || size < MIN_MDNS_SIZE) {
		start_receive();
		return;
	}
	deb2(cout << "\nodebrałem zapytanie mDNS\n";)

	// if message sent from different port then 5353, we will be
	// sending legacy unicast response (for queries) or ignore it
	// (for responses).
	if (remote_endpoint_.port() == MDNS_PORT_NUM)
		receive_universal(recv_buffer_, size, false, true);
	else
		receive_universal(recv_buffer_, size, false, false);
	
	start_receive();
}

void mdns_server::receive_universal(char* buffer, size_t size, bool via_unicast, bool mdns_port) {
	size_t end;
	
	mdns_header mdns_header_;
	mdns_header_.read(buffer);
	end = 12; // we have read 12 bytes so far

	vector<string> qname;
	try {
		qname = read_name(buffer, end, size);
	} catch (too_small_exception& ex) {
		return;
	}
		
	service service_;
	if (!mdns_header_.QR()) {

		// answering query (QR set to 0)
		mdns_query_end mdns_query_end_;
		try {
			mdns_query_end_.read(buffer, end, size);
		} catch (too_small_exception& ex) {
			return;
		}
		
		// if source port is different than 5353 mDNS port, we send
		// legacy unicast response 
		bool legacy_unicast = false;
		if (!mdns_port) {
			legacy_unicast = true;
			if (!via_unicast)
				receiver_port = remote_endpoint_.port();
			// we have to repeat ID from query in response
			id_from_query = mdns_header_.id();
		}

		// check if QU bit in query's class was set. If yes (or if we
		// received query via unicast), we will send respone via unicast.
		// For QU we do not send ever multicast response due to the
		// specification for the task (not so usual mDNS).
		bool send_via_unicast = via_unicast;
		if (!via_unicast && ((mdns_query_end_.class_() & 0xA000) || legacy_unicast)) {
			send_via_unicast = true;
			receiver_address = remote_endpoint_.address();
		}

		deb2(if (send_via_unicast)
			cout << "wysyłam przez unicast\n";)
		
		// when our name is not set, we do not want to
		// be visible for network
		if (NAME_IS_SET) {
			if (mdns_query_end_.type() == dns_type::A) {
				if (qname[0] == my_name && (service_ = which_my_service(qname, 1)) != service::NONE) {
					send_response(dns_type::A, service_, send_via_unicast, legacy_unicast);
				}
			} else if (mdns_query_end_.type() == dns_type::PTR) {
				if ((service_ = which_my_service(qname, 0)) != service::NONE) {
					send_response(dns_type::PTR, service_, send_via_unicast, legacy_unicast);
				}
			}
		}
	// we haven't send any queries via unicast, so we ignore replies
	// via unicast. We also silently ignore responces with source port
	// different than 5353 mDNS port.
	} else if (!via_unicast && mdns_port) {
		// handling response to query (QR set to 1)
		mdns_answer mdns_answer_;
		mdns_answer_.read(recv_buffer_, end, size);
		if (mdns_answer_.type() == dns_type::A) {
			handle_a_response(mdns_answer_, qname, end, size);
		} else if (mdns_answer_.type() == dns_type::PTR) {
			handle_ptr_response(mdns_answer_, qname, end, size);
		}
	}	
}

void mdns_server::handle_ptr_response(mdns_answer& mdns_answer_, vector<string> qname, size_t start, size_t size) {
	vector<string> fqdn;
	try {
		fqdn = read_compressable_name(recv_buffer_, start, size);	
	} catch (too_small_exception& ex) {
		return;
	} catch (bad_compression_exception& ex) {
		return;
	}
	mdns_client_->send_query(dns_type::A, fqdn);
}

void mdns_server::handle_a_response(mdns_answer& mdns_answer_, vector<string> qname, size_t start, size_t size) {
	// maybe someone announce, that he already uses the name, that we
	// were trying to use
	if (!NAME_IS_SET) {
		vector<string> name1 = {my_name,  "_opoznienia", "_udp", "local"};
		vector<string> name2 = {my_name,  "_ssh", "_tcp", "local"};
		
		if (qname == name1 || qname == name2)
			name_server_->notify();
	}

	if (mdns_answer_.length() != 4) return;
	
	// getting IP address
	ipv4_address address;
	try {
		address.read(recv_buffer_, start, size);
	} catch (too_small_exception& ex) {
		return;
	}
	
	// if not already existing, creating new computer
	if (!computers.count(address.address())) {
		deb3(cout << "dodaję nowy komputer\n";)
		boost::shared_ptr<computer> comp(new computer(address.address(), qname, mdns_answer_.ttl()));
		computers.insert(make_pair(address.address(), comp));
		deb3(cout << "dodałem nowy komputer\n";)
	} else {
		// maybe we need to update another service
		computers[address.address()]->add_service(qname, mdns_answer_.ttl());
	}
}

void mdns_server::send_response(dns_type type_, service service_, bool send_via_unicast, bool legacy_unicast) {
	deb(cout << "zaczynam wysyłać odpowiedź na mdns\n";)
	
	udp::endpoint receiver_endpoint;

	if (send_via_unicast)
		receiver_endpoint.address(receiver_address);
	else
		receiver_endpoint.address(boost::asio::ip::address::from_string("224.0.0.251"));

	if (legacy_unicast)
		receiver_endpoint.port(receiver_port);
	else
		receiver_endpoint.port(MDNS_PORT_NUM);

	std::ostringstream oss;
	std::vector<boost::asio::const_buffer> buffers;
	
	// ID, Flags (84 00 for response), QDCOUNT, ANCOUNT, NSCOUNT, ARCOUNT
	mdns_header mdns_header_;
	if (legacy_unicast)
		mdns_header_.id(id_from_query);
	else
		mdns_header_.id(0);
	mdns_header_.flags(RESPONSE_FLAG);
	mdns_header_.qdcount(0);
	mdns_header_.ancount(1);
	mdns_header_.nscount(0);
	mdns_header_.arcount(0);
	oss << mdns_header_;
	
	// crete FQDN appropriate for this service
	vector<string> fqdn = {my_name};
	if (service_ == service::UDP) {
		fqdn.push_back("_opoznienia");
		fqdn.push_back("_udp");
	} else if (service_ == service::TCP) {
		fqdn.push_back("_ssh");
		fqdn.push_back("_tcp");
	}
	fqdn.push_back("local");

	// FQDN specified by a list of component strings.		
	ostringstream oss_fqdn;
	for (size_t i = 0; i < fqdn.size(); i++) {
		deb(cout << "!!!" << fqdn[i].length() << " " << fqdn[i] << "\n";)

		uint8_t len = static_cast<uint8_t>(fqdn[i].length());
		oss_fqdn << len << fqdn[i];
		// In PTR, QNAME is not full name, but the same name as in query
		if ((type_ != dns_type::PTR) || i != 0)
			oss << len << fqdn[i];
	}

	// terminating FQDN and QNAME with null byte
	oss << static_cast<uint8_t>(0);
	oss_fqdn << static_cast<uint8_t>(0);

	mdns_answer mdns_answer_;
	mdns_answer_.type(type_);
	if (legacy_unicast)
		// the cache-flush bit MUST NOT be set in legacy unicast responses
		mdns_answer_.class_(0x0001);
	else
		mdns_answer_.class_(0x8001);

	if (!legacy_unicast)
		// TTL equals twice as much as sending mDNS queries frequency 
		mdns_answer_.ttl(2 * static_cast<uint32_t>(exploration_time));
	else
		 //TTL given in a legacy unicast response SHOULD NOT be
		 //greater than ten seconds
		mdns_answer_.ttl(min(2 * static_cast<uint32_t>(exploration_time), static_cast<uint32_t>(10)));
			
	if (type_ == dns_type::A) {
		// IPv4 address record
		mdns_answer_.length(4);
		ipv4_address address_;
		address_.address(my_address);
		oss << mdns_answer_;
		oss << address_;

		boost::shared_ptr<std::string> message(new std::string(oss.str()));

		socket_mdns->async_send_to(boost::asio::buffer(*message), receiver_endpoint,
		  boost::bind(&mdns_server::handle_send, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
		
	} else { // PTR
		#ifdef COMPRESS_PTR
			mdns_answer_.length(fqdn[0].size() + 2); //oss_fqdn.str().length());
			oss << mdns_answer_;
			oss << static_cast<char>(fqdn[0].size()) << fqdn[0];
			oss << static_cast<char>(0xC0) << static_cast<char>(12);
		#else
			mdns_answer_.length(oss_fqdn.str().length());
			oss << mdns_answer_;
			oss << (oss_fqdn.str());
		#endif

		// we might be not the only one responding, so, according to
		// RFC 6762 chapter 6., we should delay response by a random
		// amount of time selected with uniform random distribution
		// in the range 20-120 ms.

		srand(time(NULL));
		uint8_t wait_time = 20 + (rand() % 101);

		deb4(cout << "czekam przez " << static_cast<int>(wait_time) << "\n";)

		boost::shared_ptr<std::string> message(new std::string(oss.str()));
		boost::shared_ptr<udp::endpoint> receiver_endpoint_to_send(new udp::endpoint(receiver_endpoint));

		auto ptr_response_timer(new boost::asio::deadline_timer(*io_service, boost::posix_time::milliseconds(wait_time)));

		ptr_response_timer->async_wait(boost::bind(&mdns_server::send_ptr_response, this,
			message,
			receiver_endpoint_to_send,
			boost::asio::placeholders::error));	
	}
	
	deb(cout << "wysłano odpowiedź na mdns \n";)
}

void mdns_server::send_ptr_response(boost::shared_ptr<string> message,
	boost::shared_ptr<udp::endpoint> receiver_endpoint, 
	const boost::system::error_code &ec) {

	deb4(cout << "skończyłem czekać\n";)

	socket_mdns->async_send_to(boost::asio::buffer(*message), *receiver_endpoint,
		  boost::bind(&mdns_server::handle_send, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
	
}



void mdns_server::handle_send(//boost::shared_ptr<std::string> /*message*/,
  const boost::system::error_code& /*error*/,
  std::size_t /*bytes_transferred*/) {
}

mdns_unicast_server::mdns_unicast_server() {
	start_receive();
}

void mdns_unicast_server::start_receive() {
	deb(cout << "czekam na unikaście...\n";)

	socket_mdns_unicast->async_receive_from(
		boost::asio::buffer(recv_buffer_), remote_endpoint_,
		boost::bind(&mdns_unicast_server::handle_receive, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void mdns_unicast_server::handle_receive(const boost::system::error_code& error,
  std::size_t size /*bytes_transferred*/) {

	if (error || size > BUFFER_SIZE || size < MIN_MDNS_SIZE) {
		start_receive();
		return;
	}
	  
	deb2(cout << "\nodebrałem zapytanie mDNS przez unicasta\n";)

	// checking, if from local network
	if ((remote_endpoint_.address().to_v4().to_ulong() & my_netmask) != (my_address & my_netmask)) {
		// if not: silently ignoring
		start_receive();
		return;
	}

	// we can set mdns_server attribute, because from now to sending
	// actions will execute without any async waiting
	mdns_server_->receiver_address = remote_endpoint_.address();
	mdns_server_->receiver_port = remote_endpoint_.port();
	if (remote_endpoint_.port() == MDNS_PORT_NUM)
		mdns_server_->receive_universal(recv_buffer_, size, true, true);
	else
		mdns_server_->receive_universal(recv_buffer_, size, true, false);

	start_receive();
}

#include <iostream>
#include <utility>
#include <thread>
#include <chrono>
#include <functional>
#include <ctime>
#include <string>
#include <cstdint>
#include <endian.h>
#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <unistd.h>

#include "boost/program_options.hpp"
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "shared.h"
#include "mdns_client.h"
#include "computer.h"
#include "mdns_fields.h"
#include "name_server.hpp"
#include "mdns_server.h"

using namespace std;

name_server* name_server_;
map<uint32_t, boost::shared_ptr<computer> > computers; // identify by IPv4 address
mdns_server* mdns_server_;

mdns_server::mdns_server() //boost::asio::io_service& io_service)
//~ : socket_(*io_service), udp::endpoint(udp::v4(), MDNS_PORT_NUM)) {
	{
	 
	//~ boost::system::error_code ec;
	//~ socket_mdns->open(udp::v4());
	
	//~ socket_mdns->set_option(boost::asio::socket_base::reuse_address(true), ec);
	//~ socket_mdns->set_option(boost::asio::ip::multicast::enable_loopback(false));

	//~ boost::asio::ip::address multicast_address = boost::asio::ip::address::from_string("224.0.0.251");
	//~ socket_mdns->set_option(boost::asio::ip::multicast::join_group(multicast_address), ec);

	//~ socket_mdns->bind(udp::endpoint(udp::v4(), MDNS_PORT_NUM));

	start_receive();
}

void mdns_server::announce_name() {
	send_response(dns_type::A, service::UDP);
	if (ssh_service)
		send_response(dns_type::A, service::TCP);
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
  std::size_t /*bytes_transferred*/) {
	if (!error || error == boost::asio::error::message_size){
		deb(cout << "\nodebrałem zapytanie mDNS\n";)

		size_t end;
		
		stringstream ss;
		ss << recv_buffer_;
		mdns_header mdns_header_;
		mdns_header_.read(recv_buffer_);
		end = 12; // we have read 12 bytes so far
		
		vector<string> qname = read_name(recv_buffer_, end);
		
		// at first: not compressed

		service service_;
		if (mdns_header_.qdcount() > 0) {
			// answering query
			mdns_query_end mdns_query_end_;
			mdns_query_end_.read(recv_buffer_, end);
			// when our name is not set, we do not want to
			// be visible for network
			if (NAME_IS_SET) {
				if (mdns_query_end_.type() == dns_type::A) {
					if (qname[0] == my_name && (service_ = which_my_service(qname, 1)) != service::NONE) {
						send_response(dns_type::A, service_);
					}
				} else if (mdns_query_end_.type() == dns_type::PTR) {
					if ((service_ = which_my_service(qname, 0)) != service::NONE) {
						send_response(dns_type::PTR, service_);
					}
				}
			}
		} else if (mdns_header_.ancount() > 0) {
			// handling response to query
			mdns_answer mdns_answer_;
			mdns_answer_.read(recv_buffer_, end);
			
			if (mdns_answer_.type() == dns_type::A) {
				handle_a_response(mdns_answer_, qname, end);
			} else if (mdns_answer_.type() == dns_type::PTR) {
				// sending query for IP
				handle_ptr_response(mdns_answer_, qname, end);
			}

		}
		
		start_receive();
	}
}

// sending response via multicast
void mdns_server::send_response(dns_type type_, service service_) {
	deb(cout << "zaczynam wysyłać odpowiedź na mdns\n";)
	try {
		udp::endpoint receiver_endpoint;
		receiver_endpoint.address(boost::asio::ip::address::from_string("224.0.0.251"));
		receiver_endpoint.port(MDNS_PORT_NUM);

		std::ostringstream oss;
		std::vector<boost::asio::const_buffer> buffers;
		
		// ID, Flags (84 00 for response), QDCOUNT, ANCOUNT, NSCOUNT, ARCOUNT
		mdns_header mdns_header_;
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
		fqdn.push_back("_local");

		deb(cout << fqdn[0] << " " << fqdn[1] << " " << fqdn[2] << " " << fqdn[2] << " " <<  "\n";)

		// FQDN specified by a list of component strings
		// in PTR we do not add full name, but the same name of
		// service, as in query
		ostringstream ss_fqdn;
		for (size_t i = 0; i < fqdn.size(); i++) {
			cout << "!!!" << fqdn[i].length() << " " << fqdn[i] << "\n";
			uint8_t len = static_cast<uint8_t>(fqdn[i].length());
			ss_fqdn << len << fqdn[i];
			if ((type_ != dns_type::PTR) || i != 0)
				oss << len << fqdn[i];
		}

		// terminating FQDN with null byte
		oss << static_cast<uint8_t>(0);
		ss_fqdn << static_cast<uint8_t>(0);


		mdns_answer mdns_answer_;
		mdns_answer_.type(type_);
		mdns_answer_.class_(0x8001);
		mdns_answer_.ttl(20); // TO CHANGE, signed???
		
		if (type_ == dns_type::A) {
			// IPv4 address record
			mdns_answer_.length(4);
			ipv4_address address_;
			address_.address(ntohl(my_address)); // może ogólnie zmienić
			oss << mdns_answer_;
			oss << address_;
		} else { // PTR
		
			mdns_answer_.length(ss_fqdn.str().length());
			oss << mdns_answer_;
			oss << (ss_fqdn.str());
		}


		boost::shared_ptr<std::string> message(new std::string(oss.str()));

		socket_mdns->async_send_to(boost::asio::buffer(*message), receiver_endpoint,
		  boost::bind(&mdns_server::handle_send, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred));
		deb(cout << "wysłano odpowiedź na mdns \n";)
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}

void mdns_server::handle_ptr_response(mdns_answer& mdns_answer_, vector<string> qname, size_t start) {
	// nie wiem do końca, póki co interesuje mnie tylko nazwa
	//TTL & length
	vector<string> fqdn = read_name(recv_buffer_, start);
	mdns_client_->send_query(dns_type::A, fqdn);
}

void mdns_server::handle_a_response(mdns_answer& mdns_answer_, vector<string> qname, size_t start) {

	if (!NAME_IS_SET) {
		vector<string> name1 = {my_name,  "_opoznienia", "_udp", "_local"};
		vector<string> name2 = {my_name,  "_ssh", "_tcp", "_local"};
		
		if (qname == name1 || qname == name2)
			name_server_->notify();
	}
	
	// getting IP address
	ipv4_address address;
	address.read(recv_buffer_, start); // where TTL and length?
	// if not already existing, creating new computer
	if (!computers.count(address.address())) {
		boost::shared_ptr<computer> comp(new computer(address.address(), qname));
		computers.insert(make_pair(address.address(), comp));
	} else {
		// maybe we need to update another service
		computers[address.address()]->add_service(qname);
	}
}

void mdns_server::handle_send(//boost::shared_ptr<std::string> /*message*/,
  const boost::system::error_code& /*error*/,
  std::size_t /*bytes_transferred*/)
{
}


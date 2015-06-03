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

using namespace std;

mdns_client* mdns_client_;
map<uint32_t, boost::shared_ptr<computer> > computers; // identify by IPv4 address

class mdns_server
{
	public:
		mdns_server() //boost::asio::io_service& io_service)
		: socket_(*io_service, udp::endpoint(udp::v4(), MDNS_PORT_NUM)) {
			
			boost::system::error_code ec;
			socket_.set_option(boost::asio::socket_base::reuse_address(true), ec);
			socket_.set_option(boost::asio::ip::multicast::enable_loopback(false));
			
			boost::asio::ip::address multicast_address = boost::asio::ip::address::from_string("224.0.0.251");
			socket_.set_option(boost::asio::ip::multicast::join_group(multicast_address), ec);

			start_receive();
		}

	private:
		void start_receive() {
			deb(cout << "czekam...\n";)
			socket_.async_receive_from(
				boost::asio::buffer(recv_buffer_), remote_endpoint_,
				boost::bind(&mdns_server::handle_receive, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}

		void handle_receive(const boost::system::error_code& error,
		  std::size_t /*bytes_transferred*/) {
			if (!error || error == boost::asio::error::message_size){
				deb(cout << "\nodebrałem zapytanie mDNS\n";)

				size_t end;
				
				stringstream ss;
				ss << recv_buffer_;
				mdns_header mdns_header_; // = read_mdns_header(recv_buffer_, end);
				//~ ss >> mdns_header_;
				mdns_header_.read(recv_buffer_);
		
				deb(cout << "odczytałem nagłówek \n";)
				deb(cout << "type: " << static_cast<int>(mdns_header_.id()) << " " <<
				static_cast<int>(mdns_header_.flags()) << " " <<
				static_cast<int>(mdns_header_.qdcount()) << " " <<
				static_cast<int>(mdns_header_.ancount()) << " " <<
				static_cast<int>(mdns_header_.nscount()) << " " <<
				static_cast<int>(mdns_header_.arcount()) << " " << "\n";)
				for (int i = 0; i < 12; i++)
					cout << static_cast<int>(recv_buffer_[i]) << " ";
				cout << "\n";
				deb(mdns_header_.pisz();)
				/*
				deb(cout << "pałowo: " << static_cast<int>(mdns_header_.id()) << " " <<
				static_cast<int>(recv_buffer_[2]) << " " <<
				static_cast<int>((recv_buffer_[3]) ) << " " <<
				static_cast<int>(recv_buffer_[4]) << " " <<
				static_cast<int>(recv_buffer_[5]) << " " <<
				static_cast<int>(recv_buffer_[6]) << " " << "\n";) */
				
				end = 12;
				uint16_t type_ = 0, class_ = 0;
				vector<string> fqdn = read_fqdn(recv_buffer_, type_, class_, end);


				// at first: not compressed

				service service_;
				// answering query
				deb(cout << "type: " << static_cast<int>(type_) << static_cast<int>(mdns_header_.flags()) << "\n";)
				if (mdns_header_.flags() == 0) {
					
					
					if (type_ == dns_type::A) {
						if (fqdn[0] == my_name && (service_ = which_my_service(fqdn, 1)) != NONE ) {
							send_response(dns_type::A, service_);
						}
					} else if (type_ == dns_type::PTR) {
						if ((service_ = which_my_service(fqdn, 0)) != NONE ) {
							send_response(dns_type::PTR, service_);
						}
					}
				} else if (mdns_header_.flags() == RESPONSE_FLAG) {
					// handling response to query
					deb(cout << "response type: " << static_cast<int>(type_);)
					if (type_ == dns_type::A) {
						handle_a_response(fqdn, end);
						
					} else if (type_ == dns_type::PTR) {
						// sending query for IP
						handle_ptr_response(fqdn, end);
						//~ if ((service_ = which_my_service(fqdn, 0)) != NONE ) {
							//~ send_response(dns_type::PTR, service_);
						//~ }
					}

				}
				
				/*
				boost::shared_ptr<std::string> message(new std::string(make_daytime_string()));

				socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
				  boost::bind(&mdns_server::handle_send, this, message,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
				*/
				start_receive();
			}
		}

		// sending response via multicast
		void send_response(dns_type type_, service service_) {
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
				
				//~ boost::shared_ptr<vector<uint16_t> > header(new vector<uint16_t>{0, htons(RESPONSE_FLAG), 0, htons(1), 0, 0});
				//~ buffers.push_back(boost::asio::buffer(*header));

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
				for (size_t i = ((type_ == dns_type::PTR) ? 1 : 0); i < fqdn.size(); i++) {
					cout << "!!!" << fqdn[i].length() << " " << fqdn[i] << "\n";
					uint8_t len = static_cast<uint8_t>(fqdn[i].length());
					oss << len << fqdn[i];
				}

				// terminating FQDN with null byte
				oss << static_cast<uint8_t>(0);
				
				//~ boost::shared_ptr<std::string> message(new std::string(oss.str()));
				//~ buffers.push_back(boost::asio::buffer(*message));
				deb(cout << "___" << oss.str() << "\n";)

				
				//~ boost::shared_ptr<vector<uint8_t> > null_byte(new vector<uint8_t>{0});
				//~ buffers.push_back(boost::asio::buffer(*null_byte));

				mdns_answer mdns_answer_;
				mdns_answer_.type(type_);
				mdns_answer_.class_(0x8001);
				mdns_answer_.ttl(20); // TO CHANGE, signed???

				//~ mdns_answer_.length(4);
				

				// IPv4 address record
				/*boost::shared_ptr<vector<uint16_t> > type_class(new vector<uint16_t>{htons(type_ ), htons(0x8001)});
				buffers.push_back(boost::asio::buffer(*type_class));

				boost::shared_ptr<vector<uint32_t> > ttl(new vector<uint32_t>{htonl(20)});
				buffers.push_back(boost::asio::buffer(*ttl));

				boost::shared_ptr<vector<uint16_t> > length(new vector<uint16_t>{htons(4)});
				buffers.push_back(boost::asio::buffer(*length));

				*/
				
				if (type_ == dns_type::A) {
					mdns_answer_.length(4);
					ipv4_address address_;
					address_.address(ntohl(my_address)); // może ogólnie zmienić
					oss << mdns_answer_;
					oss << address_;
				} else { // PTR
					mdns_answer_.length(my_name.length());
					oss << mdns_answer_;
					oss << my_name;
				}
				

				//~ boost::shared_ptr<vector<uint32_t> > add(new vector<uint32_t>{my_address});
				//~ buffers.push_back(boost::asio::buffer(*add));

				boost::shared_ptr<std::string> message(new std::string(oss.str()));
				
				//~ socket_.async_send_to( buffers, receiver_endpoint,
				socket_.async_send_to(boost::asio::buffer(*message), receiver_endpoint,
				  boost::bind(&mdns_server::handle_send, this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
				deb(cout << "wysłano odpowiedź na mdns \n";)
			}
			catch (std::exception& e) {
				std::cerr << e.what() << std::endl;
			}
		}

		void handle_ptr_response(vector<string> fqdn, size_t start) {
			// nie wiem do końca, póki co interesuje mnie tylko nazwa
			mdns_client_->send_query(dns_type::A, fqdn);
		}

		void handle_a_response(vector<string> fqdn, size_t start) {
			// getting IP address
			ipv4_address address;
			address.read(recv_buffer_, start + 6); // where TTL and length?
			// if not already existing, creating new computer
			if (!computers.count(address.address())) {
				boost::shared_ptr<computer> comp(new computer(address.address(), fqdn));
				computers.insert(make_pair(address.address(), comp));
			} else {
				// maybe we need to another service
				computers[address.address()]->add_service(fqdn);
			}
		}

		void handle_send(//boost::shared_ptr<std::string> /*message*/,
		  const boost::system::error_code& /*error*/,
		  std::size_t /*bytes_transferred*/)
		{
		}

		udp::socket socket_;
		udp::endpoint remote_endpoint_;
		char recv_buffer_[BUFFER_SIZE];
		
};

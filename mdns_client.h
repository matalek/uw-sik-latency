#ifndef MDNS_CLIENT_H
#define MDNS_CLIENT_H

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

#include "boost/program_options.hpp"
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "shared.h"
#include "mdns_fields.h"

using boost::asio::ip::udp;

using namespace std;



class mdns_client {
	public:

		mdns_client() : //socket_(*io_service),
		timer_(*io_service, boost::posix_time::seconds(exploration_time)) { //, udp::endpoint(udp::v4(), MDNS_PORT_NUM)) {
			//~ boost::system::error_code ec;
			//~ socket_mdns->open(udp::v4());
			//~ socket_mdns->set_option(boost::asio::socket_base::reuse_address(true), ec);
			//~ socket_mdns->bind(udp::endpoint(udp::v4(), MDNS_PORT_NUM));
			timer_.async_wait(boost::bind(&mdns_client::ask_for_services, this));
		}

	
	
		void send_query(dns_type type_, vector<string> fqdn) {
			deb(cout << "\nzaczynam wysyłać mdnsa typu " << type_ << " \n";)
			try {

				udp::endpoint receiver_endpoint;
				receiver_endpoint.address(boost::asio::ip::address::from_string("224.0.0.251"));
				receiver_endpoint.port(MDNS_PORT_NUM);

				std::ostringstream oss;
				std::vector<boost::asio::const_buffer> buffers;
				
				// ID, Flags (0 for query), QDCOUNT, ANCOUNT, NSCOUNT, ARCOUNT
				mdns_header mdns_header_;
				mdns_header_.id(0);
				mdns_header_.flags(0);
				mdns_header_.qdcount(1);
				mdns_header_.ancount(0);
				mdns_header_.nscount(0);
				mdns_header_.arcount(0);
				oss << mdns_header_;

				// FQDN specified by a list of component strings
				for (size_t i = 0; i < fqdn.size(); i++) {
					uint8_t len = static_cast<uint8_t>(fqdn[i].length());
					oss << len << fqdn[i];
				}

				// terminating FQDN with null byte
				oss << static_cast<uint8_t>(0);

				// QTYPE & QCLASS (00 01 for Internet)
				mdns_query_end mdns_query_end_;
				mdns_query_end_.type(type_);
				mdns_query_end_.class_(1);
				oss << mdns_query_end_;


				boost::shared_ptr<std::string> message(new std::string(oss.str()));
				
				socket_mdns->async_send_to(
				//~ buffers,
				boost::asio::buffer(*message),
				 receiver_endpoint,
				  boost::bind(&mdns_client::handle_send, this, //message,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));

			}
			catch (std::exception& e) {
				std::cerr << e.what() << std::endl;
			}
		}

	private:
	
		void ask_for_services() {
			vector <string> fqdn = { "_opoznienia", "_udp", "_local"};
			send_query(dns_type::PTR, fqdn);

			fqdn = { "_ssh", "_tcp", "_local"};
			send_query(dns_type::PTR, fqdn);
			
			timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(exploration_time));
			timer_.async_wait(boost::bind(&mdns_client::ask_for_services, this));
		}

		void handle_send(//boost::shared_ptr<std::string> /*message*/,
			const boost::system::error_code& /*error*/,
			std::size_t /*bytes_transferred*/) {
				
			deb(cout << "wysłano!!!\n";)
		}

		//~ udp::socket socket_;
		boost::asio::deadline_timer timer_;
		

};


#endif

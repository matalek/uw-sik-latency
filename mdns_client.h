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

using boost::asio::ip::udp;

using namespace std;



class mdns_client
{
	public:

		mdns_client() //boost::asio::io_service& io_service)
		: socket_(*io_service) { //, udp::endpoint(udp::v4(), MDNS_PORT_NUM)) {
			boost::system::error_code ec;
			socket_.set_option(boost::asio::socket_base::reuse_address(true), ec);
			socket_.open(udp::v4());
		}

		void send_query(dns_type type_, vector<string> fqdn) {
			deb(cout << "\nzaczynam wysyłać mdnsa\n";)
			try {

				udp::endpoint receiver_endpoint;
				receiver_endpoint.address(boost::asio::ip::address::from_string("224.0.0.251"));
				receiver_endpoint.port(MDNS_PORT_NUM);

				std::ostringstream oss;
				std::vector<boost::asio::const_buffer> buffers;
				
				// ID, Flags (0 for query), QDCOUNT, ANCOUNT, NSCOUNT, ARCOUNT
				boost::shared_ptr<vector<uint16_t> > header(new vector<uint16_t>{0, 0, htons(1), 0, 0, 0});
				buffers.push_back(boost::asio::buffer(*header));

				
				//~ for (size_t i = 0; i < header.size(); i++)
					//~ oss << header[i];

				// FQDN specified by a list of component strings
				for (size_t i = 0; i < fqdn.size(); i++) {
					uint8_t len = static_cast<uint8_t>(fqdn[i].length());
					oss << len << fqdn[i];
				}

				boost::shared_ptr<std::string> message(new std::string(oss.str()));
				buffers.push_back(boost::asio::buffer(*message));

				// terminating FQDN with null byte
				boost::shared_ptr<vector<uint8_t> > null_byte(new vector<uint8_t>{0});
				buffers.push_back(boost::asio::buffer(*null_byte));
				
				// QTYPE & QCLASS (00 01 for Internet)
				boost::shared_ptr<vector<uint16_t> > type_class(new vector<uint16_t>{htons(type_), htons(1)});
				buffers.push_back(boost::asio::buffer(*type_class));

				socket_.async_send_to(
				buffers,
				//~ boost::asio::buffer(*message),
				 receiver_endpoint,
				  boost::bind(&mdns_client::handle_send, this, //message,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));

				//~ deb(cout << "wysłano mdns" << oss.str() << "\n";)
			}
			catch (std::exception& e) {
				std::cerr << e.what() << std::endl;
			}
		}

		

	private:


		void handle_send(//boost::shared_ptr<std::string> /*message*/,
		  const boost::system::error_code& /*error*/,
		  std::size_t /*bytes_transferred*/)
		{
			deb(cout << "wysłano!!!\n";)
		}

		udp::socket socket_;

};


#endif

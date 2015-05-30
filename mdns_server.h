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
#include "mdns_client.h"

using boost::asio::ip::udp;

#define deb(a) a

using namespace std;

class mdns_server
{
	public:
		mdns_server(boost::asio::io_service& io_service)
		: socket_(io_service, udp::endpoint(udp::v4(), MDNS_PORT_NUM)) {
			
			boost::system::error_code ec;
			socket_.set_option(boost::asio::socket_base::reuse_address(true), ec);
			
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
				deb(cout << "odebrałem zapytanie mDNS\n";)

				size_t end;
				mdns_header mdns_header_ = read_mdns_header(recv_buffer_, end);

				vector<string> fqdn = read_fqdn(recv_buffer_, end);

				// at first: not compressed

				// answering A type query
				if (mdns_header_.flags == 0) {
					if (fqdn == my_name) {
						// sending response via multicast
						send_response(dns_type::A);
						
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

		void send_response(dns_type type) {
			deb(cout << "zaczynam wysyłać odpowiedź na mdns\n";)
			try {
				/*
				udp::endpoint receiver_endpoint;
				receiver_endpoint.address(boost::asio::ip::address::from_string("224.0.0.251"));
				receiver_endpoint.port(MDNS_PORT_NUM);

				//~ boost::shared_ptr<std::string> message(new std::string());

				std::ostringstream oss;
				
				// ID, Flags (0 for query), QDCOUNT, ANCOUNT, NSCOUNT, ARCOUNT
				vector<unsigned short int> header = {0, 0, htons(1), 0, 0, 0};
				std::vector<boost::asio::const_buffer> buffers;
				buffers.push_back(boost::asio::buffer(header));
				
				//~ for (size_t i = 0; i < header.size(); i++)
					//~ oss << header[i];

				// FQDN specified by a list of component strings
				for (size_t i = 0; i < fqdn.size(); i++) {
					uint8_t len = static_cast<uint8_t>(fqdn[i].length());
					oss << len << fqdn[i];
				}
				buffers.push_back(boost::asio::buffer(oss.str()));

				// terminating FQDN with null byte
				uint8_t null_byte = 0;
				buffers.push_back(boost::asio::buffer(&null_byte, 1));
				//~ oss << null_byte;
				
				// QTYPE (00 01 for a host address query) & QCLASS (00 01 for Internet)
				uint16_t flags[] = {htons(type), htons(1)};
				buffers.push_back(boost::asio::buffer(flags));
				//~ oss << htons(type) << htons(1);
				
				//~ boost::shared_ptr<std::string> message(new std::string(oss.str()));

				socket_.async_send_to(
				buffers,
				//~ boost::asio::buffer(*message),
				 receiver_endpoint,
				  boost::bind(&mdns_client::handle_send, this, //message,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
				*/
				deb(cout << "wysłano odpowiedź na mdns \n";)
			}
			catch (std::exception& e) {
				std::cerr << e.what() << std::endl;
			}
		}

		void handle_send(boost::shared_ptr<std::string> /*message*/,
		  const boost::system::error_code& /*error*/,
		  std::size_t /*bytes_transferred*/)
		{
		}

		udp::socket socket_;
		udp::endpoint remote_endpoint_;
		//~ boost::array<char, BUFFER_SIZE> recv_buffer_;
		char recv_buffer_[BUFFER_SIZE];
};

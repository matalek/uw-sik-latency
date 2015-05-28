#include <iostream>
#include <utility>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>
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

#define deb(a) a

using namespace std;


class mdns_server
{
	public:
		mdns_server(boost::asio::io_service& io_service)
		: socket_(io_service, udp::endpoint(udp::v4(), MDNS_PORT_NUM)) {
			boost::system::error_code ec;
			socket_.set_option(boost::asio::socket_base::reuse_address(true), ec);
			start_receive();
		}

	private:
		void start_receive() {
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

				std::istringstream iss(recv_buffer_);
				mdns_header mdns_header_ = read_mdns_header(iss);

				vector<string> fqdn = read_fqdn(iss);
				//~ iss >> tak;
				//~ deb(cout << iss.str() << " " << tak  << "\n";)

				
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

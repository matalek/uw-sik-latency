#ifndef UI_H
#define UI_H

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
using boost::asio::ip::tcp;

class tcp_connection : public boost::enable_shared_from_this<tcp_connection>{
	public:
		typedef boost::shared_ptr<tcp_connection> pointer;

		static pointer create() {
		//boost::asio::io_service& io_service){
			return pointer(new tcp_connection());//*io_service));
		}

		tcp::socket& socket() {
			return socket_;
		}

		void start() {
			// will echo, will suppress go ahead
			uint8_t message_[6] = {255, 251, 1, 255, 251, 3};

			//~ lines.clear();
			//~ for (int i = 0; i < 30; i++) {
				//~ std::ostringstream oss;
				//~ oss << "linia " << i;
				//~ cout << oss.str() << "\n";
				//~ lines.push_back(oss.str());
			//~ }
			boost::asio::async_write(socket_, boost::asio::buffer(message_),
				boost::bind(&tcp_connection::handle_start, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}

		void refresh() {
			lines.clear();
			for (auto it = computers.begin(); it != computers.end(); it++) {
				std::ostringstream oss;
				// TO DO: ip address
				oss << it->second->get_name() << " " << it->second->get_udp_average()
					<< " " << it->second->get_tcp_average()
					<< " " << it->second->get_icmp_average();
				lines.push_back(oss.str());
			}
			write_lines();
		}

	private:

		void write_lines() {
			message_ = "\033[2J\033[0;0H"; // clean console
			unsigned int end = lines.size();
			if (start_line + MAX_LINES < end)
				end = start_line + MAX_LINES;
			
			for (unsigned int i = start_line; i < end; i++)
				message_ += lines[i] + ((i != end - 1) ? "\n\r" : "");

			boost::shared_ptr<std::string> message_to_send(new std::string(message_));
			
			boost::asio::async_write(socket_, boost::asio::buffer(*message_to_send),
				boost::bind(&tcp_connection::handle_write, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}

		

		void write(char c) {
			bool has_changed = false;
			
			switch(c) {
				case 'a':
					if (start_line < lines.size() - 1) {
						start_line++;
						has_changed = true;
					}
					break;
				case 'q':
					if (start_line > 0) {
						start_line--;
						has_changed = true;
					}
					break;
			}

			if (has_changed) {
				write_lines();
			} else
				read();
		}

		void read() {

			boost::asio::async_read_until(socket_, response_, "",
          boost::bind(&tcp_connection::handle_read, shared_from_this(),
            boost::asio::placeholders::error));

		}

		tcp_connection()
		//boost::asio::io_service& io_service)
		: socket_(*io_service) {
			start_line = 0;
		}

		void handle_start(const boost::system::error_code& /*error*/,
		  size_t /*bytes_transferred*/) { refresh(); }

		void handle_write(const boost::system::error_code& /*error*/,
		  size_t /*bytes_transferred*/) { read(); }

		void handle_read(const boost::system::error_code& /*error*/) {
			std::istream response_stream(&response_);
			char c;
			response_stream >> c;
			//cout << &response_;
			  write(c); }

		tcp::socket socket_;
		std::string message_;
		boost::asio::streambuf response_;

		vector<string> lines;
		unsigned int start_line;
};

class tcp_server {
	public:
		tcp_server(//boost::asio::io_service& io_service,
		uint16_t ui_port_num)
		: acceptor_(*io_service, tcp::endpoint(tcp::v4(), ui_port_num)) {
			start_accept();
		}

	private:
		void start_accept() {
			tcp_connection::pointer new_connection =
			  tcp_connection::create();//acceptor_.get_io_service());

			acceptor_.async_accept(new_connection->socket(),
				boost::bind(&tcp_server::handle_accept, this, new_connection,
				boost::asio::placeholders::error));
		}

		void handle_accept(tcp_connection::pointer new_connection,
		  const boost::system::error_code& error) {

			if (!error)
				new_connection->start();

			start_accept();
		}

		tcp::acceptor acceptor_;
};

#endif

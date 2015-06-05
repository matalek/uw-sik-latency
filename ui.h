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

#include "shared.h"

using boost::asio::ip::udp;
using boost::asio::ip::tcp;

// out of 80: 15 reserved for IP address and 17 (= 3*  5 + 2) for
// printing data 
#define FREE_SPACES 43

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

			timer_.async_wait(boost::bind(&tcp_connection::handle_timer, this));

			boost::asio::async_write(socket_, boost::asio::buffer(message_),
				boost::bind(&tcp_connection::handle_start, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}

		void refresh() {
			lines.clear();
			for (auto it = computers.begin(); it != computers.end(); it++) {
				std::ostringstream oss;

				uint32_t avg[] = {it->second->get_udp_average(),
					it->second->get_tcp_average(),
					it->second->get_icmp_average()};

				uint32_t sum = 0;
				uint8_t cnt = 0;

				for (int i = 0; i < 3; i++)
					if (avg[i]) {
						sum += avg[i];
						cnt++;
					}
					
				uint8_t spaces;
				uint32_t avg_avg = MAX_DELAY * 1000;
				if (cnt) {
					avg_avg = sum / cnt;
					spaces =  static_cast<uint8_t>((avg_avg * FREE_SPACES)/ (MAX_DELAY * 1000));
				} else
					spaces = FREE_SPACES;
				
				oss << it->second->get_address_string();

				for (uint8_t i = 0; i < spaces; i++)
					oss << " ";
				
				oss << avg[0] << " " << avg[1] << " " << avg[2];
				lines.push_back(make_pair(avg_avg, oss.str()));
			}
			write_lines();
			
		}

	private:
		
		void write_lines() {
			message_ = "\033[2J\033[0;0H"; // clean console
			unsigned int end = lines.size();
			if (start_line + MAX_LINES < end)
				end = start_line + MAX_LINES;

			sort(lines.begin(), lines.end());
			
			for (unsigned int i = start_line; i < end; i++)
				message_ += lines[i].second + ((i != end - 1) ? "\n\r" : "");

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
			: socket_(*io_service), timer_(*io_service, boost::posix_time::seconds(ui_refresh_time)) {
			start_line = 0;
		}

		void handle_timer() {
			refresh();
			
			timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(ui_refresh_time));
			timer_.async_wait(boost::bind(&tcp_connection::handle_timer, this));

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
			write(c);
		}

		tcp::socket socket_;
		std::string message_;
		boost::asio::streambuf response_;
		boost::asio::deadline_timer timer_;

		vector<pair <uint32_t, string> > lines;
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

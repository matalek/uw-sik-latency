#ifndef ICMP_CLIENT_H
#define ICMP_CLIENT_H

#include "shared.h"
#include "icmp_header.hpp"
#include "ipv4_header.hpp"

class icmp_client {
	public:
		icmp_client();
	
		void measure(uint32_t address);

	private:
		uint64_t get_time();

		void start_icmp_receive();

		void handle_icmp_receive(const boost::system::error_code& ec, std::size_t length);

		map<uint16_t, uint64_t> icmp_start_times;
		icmp::socket socket_icmp;
		uint16_t sequence_number;
		boost::asio::streambuf icmp_reply_buffer;
};

extern icmp_client* icmp_client_;

#endif

#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include "shared.h"

void udp_delay_server() {
	try {
		
		boost::asio::io_service my_io_service;

		udp::socket socket(my_io_service, udp::endpoint(udp::v4(), udp_port_num));

		for (;;) {
			boost::array<uint64_t, 1> recv_buf;
			udp::endpoint remote_endpoint;
			boost::system::error_code error;
			socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint, 0, error);

			if (error && error != boost::asio::error::message_size)
				throw boost::system::system_error(error);

			// calculating current time
			struct timeval tv;
			gettimeofday(&tv,NULL);
			uint64_t time = 1000000 * tv.tv_sec + tv.tv_usec;

			// creating message
			boost::array<uint64_t, 2> message;
			message[0] = recv_buf[0];
			message[1] = htobe64(time);

			boost::system::error_code ignored_error;
			socket.send_to(boost::asio::buffer(message),
				remote_endpoint, 0, ignored_error);
		}
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

}

#endif

#ifndef MDNS_SERVER_H
#define MDNS_SERVER_H

#include "shared.h"
#include "name_server.hpp"
#include "computer.h"

using namespace std;

extern map<uint32_t, boost::shared_ptr<computer> > computers; // identify by IPv4 address

class mdns_server
{
	public:
		mdns_server();

		void announce_name();

	private:
		void start_receive();

		void handle_receive(const boost::system::error_code& error,
		  std::size_t /*bytes_transferred*/);

		// sending response via multicast
		void send_response(dns_type type_, service service_);

		void handle_ptr_response(mdns_answer& mdns_answer_, vector<string> qname, size_t start);

		void handle_a_response(mdns_answer& mdns_answer_, vector<string> qname, size_t start);

		void handle_send(//boost::shared_ptr<std::string> /*message*/,
		  const boost::system::error_code& /*error*/,
		  std::size_t /*bytes_transferred*/);

		udp::endpoint remote_endpoint_;
		char recv_buffer_[BUFFER_SIZE];
};

extern mdns_server* mdns_server_;

#endif

#ifndef MDNS_SERVER_H
#define MDNS_SERVER_H

#include "shared.h"
#include "name_server.hpp"
#include "computer.h"

using namespace std;

extern map<uint32_t, boost::shared_ptr<computer> > computers; // identify by IPv4 address

class mdns_server {
	public:
		mdns_server();

		void announce_name();

		void receive_universal(char* buffer, size_t size, bool via_unicast, bool mdns_port);

		boost::asio::ip::address receiver_address;
		uint16_t receiver_port;
		
	private:
		void start_receive();

		void handle_receive(const boost::system::error_code& error,
		  std::size_t /*bytes_transferred*/);

		// sending response via multicast or unicast (if appropriate
		// conditions are met)
		void send_response(dns_type type_, service service_, bool send_via_unicast, bool legacy_unicast);

		void send_ptr_response(boost::shared_ptr<string> message,
			boost::shared_ptr<udp::endpoint> receiver_endpoint, 
			const boost::system::error_code &ec);
		
		void handle_ptr_response(mdns_answer& mdns_answer_, vector<string> qname, size_t start, size_t size);

		void handle_a_response(mdns_answer& mdns_answer_, vector<string> qname, size_t start, size_t size);

		void handle_send(//boost::shared_ptr<std::string> /*message*/,
		  const boost::system::error_code& /*error*/,
		  std::size_t /*bytes_transferred*/);

		udp::endpoint remote_endpoint_;
		char recv_buffer_[BUFFER_SIZE];
		uint16_t id_from_query;
};

extern mdns_server* mdns_server_;

class mdns_unicast_server {
	public:
		mdns_unicast_server();

	private:
		void start_receive();

		void handle_receive(const boost::system::error_code& error,
		  std::size_t /*bytes_transferred*/);

		udp::endpoint remote_endpoint_;
		char recv_buffer_[BUFFER_SIZE];
};

extern mdns_unicast_server* mdns_unicast_server_;



#endif

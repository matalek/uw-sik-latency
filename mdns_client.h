#ifndef MDNS_CLIENT_H
#define MDNS_CLIENT_H

#include "shared.h"
#include "mdns_fields.h"

using namespace std;

class mdns_client {
	public:

		mdns_client() : timer_(*io_service) {
			ask_for_services();
		}

		void send_query(dns_type type_, vector<string> fqdn) {
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
			boost::asio::buffer(*message),
			 receiver_endpoint,
			  boost::bind(&mdns_client::handle_send, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}

	private:
	
		void ask_for_services() {
			vector <string> fqdn = { "_opoznienia", "_udp", "local"};
			send_query(dns_type::PTR, fqdn);

			fqdn = { "_ssh", "_tcp", "local"};
			send_query(dns_type::PTR, fqdn);
			
			timer_.expires_from_now(boost::posix_time::seconds(exploration_time));
			timer_.async_wait(boost::bind(&mdns_client::ask_for_services, this));
		}

		void handle_send(//boost::shared_ptr<std::string> /*message*/,
			const boost::system::error_code& /*error*/,
			std::size_t /*bytes_transferred*/) { }

		boost::asio::deadline_timer timer_;
};


#endif

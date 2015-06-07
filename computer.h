#ifndef COMPUTER_H
#define COMPUTER_H

#include "shared.h"

class computer : public boost::enable_shared_from_this<computer> {
	public:
		
		computer(uint32_t add, vector<string>& fqdn, uint32_t ttl);

		~computer();
		
		void add_service(vector<string>& fqdn, uint32_t ttl);

		void measure();

		void notify_icmp(uint64_t res);
		
		string get_name();
		
		uint32_t get_udp_average();

		uint32_t get_tcp_average();

		uint32_t get_icmp_average();

		string get_address_string();

		// returns, if any service is still valid for this computer
		bool verify_ttl();

	private:
		// calculating current time
		uint64_t get_time();

		void measure_udp();

		void handle_send_udp(const boost::system::error_code& ec/*error*/,
			std::size_t /*bytes_transferred*/);
		
		void handle_receive_udp(const boost::system::error_code& ec/*error*/,
		  std::size_t /*bytes_transferred*/);

		void measure_tcp();

		void handle_connect_tcp(boost::shared_ptr<uint64_t> start_time,
			boost::shared_ptr<tcp::socket> socket_tcp,
			const boost::system::error_code& ec /*error*/);

		void measure_icmp();

		string name;
		bool opoznienia_service;
		bool ssh_service; 
		uint32_t address; // host order

		uint64_t leave_time_opoznienia;
		uint64_t leave_time_ssh;

		udp::socket socket_udp;
		udp::endpoint remote_udp_endpoint;

		tcp::endpoint remote_tcp_endpoint;
		
		char recv_buffer_[BUFFER_SIZE];

		queue<uint32_t> udp_times;
		queue<uint32_t> tcp_times;
		queue<uint32_t> icmp_times;

		uint32_t udp_sum;
		uint32_t tcp_sum;
		uint32_t icmp_sum;
};

extern map<uint32_t, boost::shared_ptr<computer> > computers;

#endif

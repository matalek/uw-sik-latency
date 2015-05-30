#ifndef COMPUTER_H
#define COMPUTER_H

#include "shared.h"

class computer : public boost::enable_shared_from_this<computer> {
	public:

		//~ computer() : socket_udp(*io_service)  { } // do zastanowienia się
		
		computer(ipv4_address add, vector<string>& fqdn) :
			socket_udp(*io_service),
			socket_tcp(*io_service) {
				
			address = add.address;
			ttl = add.ttl;
			name = fqdn[0];
			add_service(fqdn);
			deb(cout << "Dodaję komputer " << name << " " << address<< "\n";)

			// creating sockets for measurement
			socket_udp.open(udp::v4());
			remote_udp_endpoint.address(boost::asio::ip::address_v4(address));
			remote_udp_endpoint.port(udp_port_num);
			
			remote_tcp_endpoint.address(boost::asio::ip::address_v4(address));
			remote_tcp_endpoint.port(SSH_PORT_NUM);

			// only temporary here
			//~ measure_udp();
			
		}

		void add_service(vector<string>& fqdn) {
			if (fqdn.size() == 4 && fqdn[1] == "_ssh" && fqdn[2] == "_tcp" && fqdn[3] == "_local")
				ssh_service = true;
		}

		void measure() {
			measure_udp();
			measure_tcp();
		}
		
		string get_name() {
			return name;
		}
		
		uint32_t get_udp_average() {
			if (udp_times.size())
				return udp_sum / udp_times.size();
			return 0; 
		}

		uint32_t get_tcp_average() {
			if (tcp_times.size())
				return tcp_sum / tcp_times.size();
			return 0; 
		}

		uint32_t get_icmp_average() {
			if (icmp_times.size())
				return icmp_sum / icmp_times.size();
			return 0; 
		}

		string get_address_string() {
			return boost::asio::ip::address_v4(address).to_string();
		}

	private:

		void measure_udp() {
			
			// calculating current time
			struct timeval tv;
			gettimeofday(&tv,NULL);
			uint64_t time = 1000000 * tv.tv_sec + tv.tv_usec;

			// creating message
			boost::shared_ptr<vector<uint64_t> > message(new vector<uint64_t>{htobe64(time)});
				
			deb(cout << "wysyłam pomiar udp\n";)

			socket_udp.async_send_to(boost::asio::buffer(*message), remote_udp_endpoint,
				boost::bind(&computer::handle_send_udp, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}

		void handle_send_udp(const boost::system::error_code& /*error*/,
			std::size_t /*bytes_transferred*/) {
			// reading
			socket_udp.async_receive_from(
				boost::asio::buffer(recv_buffer_), remote_udp_endpoint,
				boost::bind(&computer::handle_receive_udp, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
			
		}
		
		void handle_receive_udp(const boost::system::error_code& /*error*/,
		  std::size_t /*bytes_transferred*/) {
			// calculating and saving time
			deb(cout << "odpowiedź na udp\n";)

				uint64_t val;
				memcpy((char *)&val, recv_buffer_, 8);
				uint64_t start_time = be64toh(val);
				
				memcpy((char *)&val, recv_buffer_ + 8, 8);
				uint64_t middle_time = be64toh(val);
				
				// time of receiving answer
				struct timeval tv;
				gettimeofday(&tv,NULL);
				uint64_t end_time = 1000000 * tv.tv_sec + tv.tv_usec;

				// writing on error output received data
				// KONIECZNE???
				cerr << start_time << " " << middle_time << "\n"; 
				uint64_t res = end_time - start_time;
				deb(cout << "Wynik pomiaru udp: " << res << " " << start_time << " " << end_time << "\n";)

				udp_times.push(res);
				udp_sum += res;
				if (udp_times.size() > 10) {
					udp_sum -= udp_times.front();
					udp_times.pop();
				}
		}

		void measure_tcp() {
			struct timeval tv;
			gettimeofday(&tv,NULL);
			tcp_start_time = 1000000 * tv.tv_sec + tv.tv_usec;

			// connect socket to the server
			socket_tcp.async_connect(remote_tcp_endpoint,
				boost::bind(&computer::handle_connect_tcp, shared_from_this(),
				boost::asio::placeholders::error));
		}

		void handle_connect_tcp(const boost::system::error_code& /*error*/) {

			// time of receiving answer
			struct timeval tv;
			gettimeofday(&tv,NULL);
			uint64_t end_time = 1000000 * tv.tv_sec + tv.tv_usec;

			uint64_t res = end_time - tcp_start_time;
			deb(cout << "Wynik pomiaru tcp: " << res << " " << tcp_start_time << " " << end_time << "\n";)

			tcp_times.push(res);
			tcp_sum += res;
			if (tcp_times.size() > 10) {
				tcp_sum -= tcp_times.front();
				tcp_times.pop();
			}
		}

	
		string name;
		bool ssh_service; 
		uint32_t address; // host order
		uint32_t ttl;

		udp::socket socket_udp;
		udp::endpoint remote_udp_endpoint;

		tcp::socket socket_tcp;
		tcp::endpoint remote_tcp_endpoint;
		
		char recv_buffer_[BUFFER_SIZE];

		queue<uint32_t> udp_times;
		queue<uint32_t> tcp_times;
		queue<uint32_t> icmp_times;

		uint32_t udp_sum;
		uint32_t tcp_sum;
		uint32_t icmp_sum;

		uint64_t tcp_start_time;
		
};



#endif

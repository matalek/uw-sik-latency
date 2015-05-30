#ifndef COMPUTER_H
#define COMPUTER_H

#include "shared.h"

class computer {
	public:

		//~ computer() : socket_udp(*io_service)  { } // do zastanowienia się
		
		computer(ipv4_address add, vector<string>& fqdn) :
			socket_udp(*io_service) {
			address = add.address;
			ttl = add.ttl;
			name = fqdn[0];
			add_service(fqdn);
			deb(cout << "Dodaję komputer " << name << " " << address<< "\n";)

			// creating sockets for measurement
			socket_udp.open(udp::v4());

			// only temporary here
			measure_udp();
			
		}

		void add_service(vector<string>& fqdn) {
			if (fqdn.size() == 4 && fqdn[1] == "_ssh" && fqdn[2] == "_tcp" && fqdn[3] == "_local")
				ssh_service = true;
		}


	private:

		void measure_udp() {
			
			// calculating current time
			struct timeval tv;
			gettimeofday(&tv,NULL);
			uint64_t time = 1000000 * tv.tv_sec + tv.tv_usec;

			// creating message
			boost::shared_ptr<vector<uint64_t> > message(new vector<uint64_t>{htobe64(time)});

			udp::endpoint receiver_endpoint;
				receiver_endpoint.address(boost::asio::ip::address_v4(address));
				//~ receiver_endpoint.address(boost::asio::ip::address::from_string("192.168.1.2"));
				
				receiver_endpoint.port(udp_port_num);
				
			deb(cout << "wysyłam pomiar udp\n";)

			socket_udp.async_send_to(boost::asio::buffer(*message), receiver_endpoint,
				boost::bind(&computer::handle_send_udp, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
		}

		void handle_send_udp(const boost::system::error_code& /*error*/,
			std::size_t /*bytes_transferred*/) {
			// reading
			
			
		}
		
		void handle_read_udp(const boost::system::error_code& /*error*/,
		  std::size_t /*bytes_transferred*/) {
			// calculating and saving time
			deb(cout << "odpowiedź na udp\n";)
			  
		}
	
		string name;
		bool ssh_service; 
		uint32_t address; // host order
		uint32_t ttl;
		udp::socket socket_udp;
		//~ udp::socket socket_tcp;


		
};



#endif

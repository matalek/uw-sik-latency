#ifndef COMPUTER_H
#define COMPUTER_H

#include "shared.h"
#include "icmp_header.hpp"
#include "ipv4_header.hpp"

class computer : public boost::enable_shared_from_this<computer> {
	public:

		//~ computer() : socket_udp(*io_service)  { } // do zastanowienia się
		
		computer(ipv4_address add, vector<string>& fqdn) :
			socket_udp(*io_service),
			socket_tcp(*io_service),
			socket_icmp(*io_service, icmp::v4()) { // czy nie lepiej jedno?
				
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

			remote_icmp_endpoint.address(boost::asio::ip::address_v4(address));

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
			measure_icmp();
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

		uint64_t get_time() {
			struct timeval tv;
			gettimeofday(&tv,NULL);
			return 1000000 * tv.tv_sec + tv.tv_usec;
		}

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

		void measure_icmp() {
			string body("\"Hello!\" from Asio ping.");

			// create an ICMP header for an echo request
			icmp_header echo_request;
			echo_request.type(icmp_header::echo_request);
			echo_request.code(0);
			echo_request.identifier(static_cast<unsigned short>(::getpid()));
			echo_request.sequence_number(++sequence_number);
			deb(cout << "nadałem numer " << sequence_number << "\n";)
			compute_checksum(echo_request, body.begin(), body.end());

			// encode the request packet
			boost::asio::streambuf request_buffer;
			std::ostream os(&request_buffer);
			os << echo_request << body;

			// send the request
			icmp_start_time = get_time();
			socket_icmp.send_to(request_buffer.data(), remote_icmp_endpoint);

			// wait up to five seconds for a reply
			//~ num_replies_ = 0;
			//~ timer_.expires_at(time_sent_ + posix_time::seconds(5));
			//~ timer_.async_wait(boost::bind(&pinger::handle_timeout, this));

			deb(cout << "wysłałem zapytanie icmp\n";)

			start_icmp_receive();
		}

		void start_icmp_receive()
		{
			// discard any data already in the buffer
			icmp_reply_buffer.consume(icmp_reply_buffer.size());

			// wait for a reply. We prepare the buffer to receive up to 64KB.
			socket_icmp.async_receive(icmp_reply_buffer.prepare(65536),
			boost::bind(&computer::handle_icmp_receive, this, _2));
		}

		void handle_icmp_receive(std::size_t length) {
			// the actual number of bytes received is committed to the buffer so that we
			// can extract it using a std::istream object.
			icmp_reply_buffer.commit(length);

			// decode the reply packet
			std::istream is(&icmp_reply_buffer);
			ipv4_header ipv4_hdr;
			icmp_header icmp_hdr;
			is >> ipv4_hdr >> icmp_hdr;

			deb(cout << "odebrałem icmp " << (int)icmp_hdr.identifier() << " " << ::getpid() << " " <<
			icmp_hdr.sequence_number() << " " << sequence_number << " " <<
			icmp_hdr.type() << " " << icmp_header::echo_reply << "\n";)

			// we can receive all ICMP packets received by the host, so we need to
			// filter out only the echo replies that match the our identifier and
			// expected sequence number.
			if (is && icmp_hdr.type() == icmp_header::echo_reply
			&& icmp_hdr.identifier() == static_cast<unsigned short>(::getpid())
			&& icmp_hdr.sequence_number() == sequence_number)
			{
				// if this is the first reply, interrupt the five second timeout.
				//~ if (num_replies_++ == 0)
				//~ timer_.cancel();

				// Print out some information about the reply packet.
				std::cout << length - ipv4_hdr.header_length()
				<< " bytes from " << ipv4_hdr.source_address()
				<< ": icmp_seq=" << icmp_hdr.sequence_number()
				<< ", ttl=" << ipv4_hdr.time_to_live()
				<< ", time=" << (get_time() - icmp_start_time) << " ms"
				<< std::endl;

				uint64_t res = get_time() - icmp_start_time;
				
				icmp_times.push(res);
				icmp_sum += res;
				if (icmp_times.size() > 10) {
					icmp_sum -= icmp_times.front();
					icmp_times.pop();
				}
			}
			start_icmp_receive();
		}

	
		string name;
		bool ssh_service; 
		uint32_t address; // host order
		uint32_t ttl;

		udp::socket socket_udp;
		udp::endpoint remote_udp_endpoint;

		tcp::socket socket_tcp;
		tcp::endpoint remote_tcp_endpoint;

		icmp::socket socket_icmp;
		icmp::endpoint remote_icmp_endpoint;
		
		char recv_buffer_[BUFFER_SIZE];

		queue<uint32_t> udp_times;
		queue<uint32_t> tcp_times;
		queue<uint32_t> icmp_times;

		uint32_t udp_sum;
		uint32_t tcp_sum;
		uint32_t icmp_sum;

		uint64_t tcp_start_time;
		uint64_t icmp_start_time;

		boost::asio::streambuf icmp_reply_buffer;
		unsigned short sequence_number;
		
};



#endif

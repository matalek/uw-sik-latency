#include <iostream>
#include <utility>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>
#include <ctime>
#include <string>
#include <cstdint>
#include <endian.h>
#include <algorithm>
#include <sstream>

#include "err.h"

#include "boost/program_options.hpp"
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

using boost::asio::ip::udp;
using boost::asio::ip::tcp;



#define deb(a) a

#define BUFFER_SIZE   1000
#define QUEUE_LENGTH     5

#define MAX_LINES 24

uint16_t udp_port_num = 3382; // configured by -u option
uint16_t ui_port_num = 3637; // configured by -U option
double measurement_time = 1.; // configured by -t option
double exploration_time = 1.; // configured by -T option
double ui_refresh_time = 1; // configured by -v option
bool ssh_service; // configured by -s option

using namespace std;


void udp_delay_server() {
	try {
		
		boost::asio::io_service io_service;

		udp::socket socket(io_service, udp::endpoint(udp::v4(), udp_port_num));

		for (;;) {
			boost::array<uint64_t, 1> recv_buf;
			udp::endpoint remote_endpoint;
			boost::system::error_code error;
			socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint, 0, error);

			cout << be64toh(recv_buf[0]) << "\n";

			if (error && error != boost::asio::error::message_size)
				throw boost::system::system_error(error);

			// calculating current time
			struct timeval tv;
			gettimeofday(&tv,NULL);
			unsigned long long time = 1000000 * tv.tv_sec + tv.tv_usec;

			// creating message
			boost::array<uint64_t, 2> message;
			message[0] = recv_buf[0];
			message[1] = htobe64(time);

			boost::system::error_code ignored_error;
			socket.send_to(boost::asio::buffer(message),
				remote_endpoint, 0, ignored_error);
		}
	} catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	cout << "wątek\n";
}

using boost::asio::ip::tcp;

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

class tcp_connection : public boost::enable_shared_from_this<tcp_connection>{
	public:
		typedef boost::shared_ptr<tcp_connection> pointer;

		static pointer create(boost::asio::io_service& io_service){
			return pointer(new tcp_connection(io_service));
		}

		tcp::socket& socket() {
			return socket_;
		}

		void start() {
			// will echo, will suppress go ahead
			uint8_t message_[6] = {255, 251, 1, 255, 251, 3};

			lines.clear();
			for (int i = 0; i < 30; i++) {
				std::ostringstream oss;
				oss << "linia " << i;
				cout << oss.str() << "\n";
				lines.push_back(oss.str());
			}
			boost::asio::async_write(socket_, boost::asio::buffer(message_),
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
				message_ = "\033[2J\033[0;0H"; // clean console
				unsigned int end = lines.size();
				if (start_line + MAX_LINES < end)
					end = start_line + MAX_LINES;
				
				for (unsigned int i = start_line; i < end; i++)
					message_ += lines[i] + ((i != end - 1) ? "\n\r" : "");
				
				boost::asio::async_write(socket_, boost::asio::buffer(message_),
					boost::bind(&tcp_connection::handle_write, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
			} else
				read();
		}

		void read() {

			boost::asio::async_read_until(socket_, response_, "",
          boost::bind(&tcp_connection::handle_read, shared_from_this(),
            boost::asio::placeholders::error));

		}

	private:
		tcp_connection(boost::asio::io_service& io_service)
		: socket_(io_service) { }

		void handle_write(const boost::system::error_code& /*error*/,
		  size_t /*bytes_transferred*/) { read(); }

		void handle_read(const boost::system::error_code& /*error*/) {
			std::istream response_stream(&response_);
			char c;
			response_stream >> c;
			//cout << &response_;
			  write(c); }

		tcp::socket socket_;
		std::string message_;
		boost::asio::streambuf response_;

		vector<string> lines;
		unsigned int start_line = 0;
};

class tcp_server {
	public:
		tcp_server(boost::asio::io_service& io_service)
		: acceptor_(io_service, tcp::endpoint(tcp::v4(), ui_port_num)) {
			start_accept();
		}

	private:
		void start_accept() {
			tcp_connection::pointer new_connection =
			  tcp_connection::create(acceptor_.get_io_service());

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


int main (int argc, char *argv[]) {
	
	// parsing arguments
	namespace po = boost::program_options;
	po::options_description desc("Dozwolone opcje");
	desc.add_options()
		(",u", po::value<int>(), "port serwera do pomiaru opóźnień przez UDP: 3382")
		(",U", po::value<int>(), "port serwera do połączeń z interfejsem użytkownika: 3637")
		(",t", po::value<int>(), "czas pomiędzy pomiarami opóźnień: 1 sekunda")
		(",T", po::value<int>(), "czas pomiędzy wykrywaniem komputerów: 10 sekund")
		(",v", po::value<int>(), "czas pomiędzy aktualizacjami interfejsu użytkownika: 1 sekunda")
		(",s", "rozgłaszanie dostępu do usługi _ssh._tcp: domyślnie wyłączone")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);    

	if (vm.count("-u"))
		udp_port_num = vm["-u"].as<int>();
	if (vm.count("-U"))
		ui_port_num = vm["-U"].as<int>();
	if (vm.count("-t"))
		measurement_time = vm["-t"].as<double>();
	if (vm.count("-T"))
		exploration_time = vm["-T"].as<double>();
	if (vm.count("-v"))
		ui_refresh_time = vm["-v"].as<double>();
	if (vm.count("-s"))
		ssh_service = true;

	// creating thread for UDP delay server
	std::thread udp_thread(udp_delay_server);

	try {
		boost::asio::io_service io_service;
		tcp_server server(io_service);
		io_service.run();
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	udp_thread.join();

	return 0;
}

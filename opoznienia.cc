#include <iostream>
#include <utility>
#include <thread>
#include <chrono>
#include <functional>
#include <ctime>
#include <string>
#include <cstdint>
#include <endian.h>
#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <unistd.h>

#include "boost/program_options.hpp"
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

// to get IP address
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "err.h"
#include "mdns_server.h"
#include "mdns_client.h"
#include "shared.h"
#include "ui.h"
#include "udp_server.h"

using boost::asio::ip::udp;
using boost::asio::ip::tcp;




using namespace std;




std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

void get_address() {
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET; //IPv4 IP address
	// we assume, that local network is connected to eth1 interface
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);
	my_address = ((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr).s_addr);
	my_address_str = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr); // for debugging
}


struct mdns_body {
	uint16_t type_;
	uint16_t class_;
	int32_t ttl_;
	uint16_t length_;
	uint32_t address_;
};

void add_mdns_body(mdns_body body, std::vector<boost::asio::const_buffer> buffers) {
	buffers.push_back(boost::asio::buffer(&body.type_, sizeof(body.type_)));
	buffers.push_back(boost::asio::buffer(&body.class_, sizeof(body.class_)));
	buffers.push_back(boost::asio::buffer(&body.ttl_, sizeof(body.ttl_)));
	buffers.push_back(boost::asio::buffer(&body.length_, sizeof(body.length_)));
	buffers.push_back(boost::asio::buffer(&body.address_, sizeof(body.address_)));
}

void set_name() {
	// dodać obsługę błędu
	char hostname[HOSTNAME_SIZE];
	gethostname(hostname, HOSTNAME_SIZE);

	deb(printf("hostname: %s\n", hostname);)
	//~ my_name = { hostname, "_opoznienia", "_udp", "_local"};
	//~ deb(cout << "---" << my_name[0] << "\n";)
	my_name = hostname;
}

int main(int argc, char *argv[]) {
	
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
	std::thread udp_delay_server_thread(udp_delay_server);

	get_address();
	deb(cout << "Adres ip: " << my_address << "\n";)

	set_name();

	
	try {
		boost::asio::io_service io_service;

		// creating tcp server for ui interface
		tcp_server server(io_service, ui_port_num);

		// only temporary here

		//~ udp::endpoint endpoint;
		//endpoint.address(boost::asio::ip::address::from_string("224.0.0.251"));
		//~ endpoint.port(5353);

		udp::socket socket(io_service);

		//udp::socket socket(io_service, udp::endpoint(udp::v4(), 13));
		//~ udp::socket socket(io_service, udp::endpoint(udp::v4(), MDNS_PORT_NUM));
		socket.open(udp::v4());
		
		// creating thread for mDNS sever
		//~ std::thread mdns_server_thread(mdns_server, ref(io_service));

		mdns_client mdns_client_(io_service);
		mdns_server mdns_server_(io_service);
	
		
		

		vector <string> fqdn = { "_opoznienia", "_udp", "_local"};
		//~ fqdn[0] = "_opoznienia";
		//~ fqdn[1] = "_udp";
		//~ fqdn[2] = "_local";
		mdns_client_.send_query(dns_type::PTR, fqdn);

		io_service.run();

		
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	udp_delay_server_thread.join(); // to change
	//~ mdns_server_thread.join(); // to change

	return 0;
}

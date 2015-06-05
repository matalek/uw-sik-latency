/*
 * Before running program execute:
 * sudo setcap cap_net_raw+eip ./opoznienia
 * in order to enable root previlages to create raw sockets.
 *
 * Additional assumptions:
 * - program does not measure delays to localhost.
 * - the computer on which the program is executed has only one
 * interface: eth0, or routing is appropriately set so that programs
 * could communicate on eth0 interface.
 * - name is assigned to the computer, not to service, it is offering
 * (names for opoznienia and ssh services should be equal).
 * - because we send mDNS regularly, we will be caching records 
 * for time to leave received in a response to A query (not PTR query).
 * Therefore, we wil be ignoring TTL send in response to PTR query.
 * It won't do us any harm, because, if TTL of PTR record were shorter
 * than for A record, we wouldn't (due to program specification) send
 * an additional A query. We would rather wait for an appropriate time
 * and then send both PTR and (after receiving answer) A query and
 * TTL time for PTR would be reset.
 * - we set TTL for twice as sending mDNS queries frequency (approximately,
 * because exploration time can be double value). Therefore, we also
 * assume, that sending mDNS queries time would be resonable (at least
 * one second).
 *
 */ 

// to get IP address
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "shared.h"
#include "mdns_server.h"
#include "mdns_client.h"
#include "udp_server.h"
#include "measurement_server.h"
#include "ui.h"

void get_address() {
	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET; //IPv4 IP address
	// we assume, that local network is connected to eth1 interface
	strncpy(ifr.ifr_name, "eth1", IFNAMSIZ-1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	
	my_address = ((((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr).s_addr);
	my_address_str = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr); // for debugging

	ioctl(fd, SIOCGIFNETMASK, &ifr);
	cout << "!!!" << inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr) << "a\n";

	close(fd);
}


void set_candidate_name() {
	// dodać obsługę błędu
	char hostname[HOSTNAME_SIZE];
	gethostname(hostname, HOSTNAME_SIZE);

	deb(printf("hostname: %s\n", hostname);)
	my_name = hostname;
}

int main(int argc, char *argv[]) {
	
	// parsing arguments
	int c;
	opterr = 0;

	while ((c = getopt (argc, argv, "u:U:t:T:v:s")) != -1)
		switch (c) {
			case 'u':
			{
				std::stringstream ss;
				ss << optarg;
				ss >> udp_port_num;
				if (!ss.eof()) {
					cerr << "Option -u requires an integer argument.\n";
					return 1;
				}
			}
				break;
			case 'U':
			{
				std::stringstream ss;
				ss << optarg;
				ss >> ui_port_num;
				if (!ss.eof()) {
					cout << "Option -U requires an integer argument.\n";
					return 1;
				}
			}
				break;
			case 't':
			{
				std::stringstream ss;
				ss << optarg;
				ss >> measurement_time;
				if (!ss.eof()) {
					cerr << "Option -t requires a double argument.\n";
					return 1;
				}
			}
				break;
			case 'T':
			{
				std::stringstream ss;
				ss << optarg;
				ss >> exploration_time;
				if (!ss.eof()) {
					cerr << "Option -T requires a double argument.\n";
					return 1;
				}
			}
				break;
			case 'v':
			{
				std::stringstream ss;
				ss << optarg;
				ss >> ui_refresh_time;
				if (!ss.eof()) {
					cerr << "Option -v requires a double argument.\n";
					return 1;
				}
			}
				break;
			case 's':
				announce_ssh_service = true;
				break;
			case '?':
				if (optopt == 'u' || optopt == 'U' || optopt == 't' || optopt == 'T' || optopt == 'v')
					cerr << "Option -" << (char)optopt << " requires an argument.\n";
				else if (isprint (optopt))
					cerr << "Unknown option `-" << (char)optopt << "'.\n";
				else
					cerr << "Unknown option character `\\x" << std::hex << optopt << "'.\n";
				return 1;
			default:
				abort();
		}

	deb2(cout << udp_port_num << " " << ui_port_num << " " << measurement_time << " " << exploration_time << " " << ui_refresh_time
		<< " " << static_cast<int>(announce_ssh_service) << "\n";)


	// creating thread for UDP delay server
	std::thread udp_delay_server_thread(udp_delay_server);

	set_candidate_name();
	get_address();
	deb(cout << "Adres ip: " << my_address << "\n";)
	
	
	try {
		io_service = new boost::asio::io_service();

		// creating tcp server for ui interface
		tcp_server server(ui_port_num);

		// creating socket for MDNS handling
		boost::system::error_code ec;
		
		socket_mdns = new udp::socket(*io_service);
		socket_mdns->open(udp::v4());
		socket_mdns->set_option(boost::asio::socket_base::reuse_address(true), ec);
		socket_mdns->set_option(boost::asio::ip::multicast::enable_loopback(false));

		// receive from multicast
		boost::asio::ip::address multicast_address = boost::asio::ip::address::from_string("224.0.0.251");
		socket_mdns->set_option(boost::asio::ip::multicast::join_group(multicast_address), ec);

		// bind to appropriate port
		socket_mdns->bind(udp::endpoint(boost::asio::ip::address::from_string("224.0.0.251"), MDNS_PORT_NUM));


		// socket for handling MDNS queries received via unicast
		socket_mdns_unicast = new udp::socket(*io_service);
		socket_mdns_unicast->open(udp::v4());
		socket_mdns_unicast->set_option(boost::asio::socket_base::reuse_address(true), ec);
		socket_mdns_unicast->set_option(boost::asio::ip::multicast::enable_loopback(false));

		// bind to appropriate port
		socket_mdns_unicast->bind(udp::endpoint(boost::asio::ip::address_v4(ntohl(my_address)), MDNS_PORT_NUM));


		mdns_client_ = new mdns_client{};
		name_server_ = new name_server{};
		mdns_server_ = new mdns_server{};
		mdns_unicast_server_ = new mdns_unicast_server{};
		measurement_server measurement_server{};
		
		io_service->run();
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	//~ udp_delay_server_thread.join(); 

	free(mdns_server_);
	free(mdns_client_);
	free(name_server_);
	free(socket_mdns);
	free(io_service);
	
	return 0;
}

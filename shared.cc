#include "shared.h"

uint16_t udp_port_num = 3382; // configured by -u option
uint16_t ui_port_num = 3637; // configured by -U option
double measurement_time = 1.; // configured by -t option
double exploration_time = 10.; // configured by -T option
double ui_refresh_time = 1; // configured by -v option
bool announce_ssh_service; // configured by -s option
bool NAME_IS_SET = false;

string my_name;
uint32_t my_address; // BE order
string my_address_str;
uint32_t my_netmask; // BE order

boost::asio::io_service* io_service;
udp::socket* socket_mdns;
udp::socket* socket_mdns_unicast;

service which_my_service(vector<string>& fqdn, size_t start) {
	if (start + 2 < fqdn.size() &&
		fqdn[start] == "_opoznienia" &&
		fqdn[start + 1] == "_udp" &&
		fqdn[start + 2] == "_local")
		return service::UDP;
	if (announce_ssh_service && start + 2 < fqdn.size() &&
		fqdn[start] == "_ssh" &&
		fqdn[start + 1] == "_tcp" &&
		fqdn[start + 2] == "_local")
		return service::TCP;
	return NONE;
}

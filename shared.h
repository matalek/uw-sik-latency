#ifndef SHARED_H
#define SHARED_H

#include <sstream>
#include <vector>
#include <string.h>
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
#include <memory>
#include <map>
#include <queue>

#include "boost/program_options.hpp"
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


using namespace std;
using boost::asio::ip::udp;
using boost::asio::ip::tcp;
using boost::asio::ip::icmp;

uint16_t udp_port_num = 3382; // configured by -u option
uint16_t ui_port_num = 3637; // configured by -U option
double measurement_time = 1.; // configured by -t option
double exploration_time = 10.; // configured by -T option
double ui_refresh_time = 1; // configured by -v option
bool ssh_service; // configured by -s option

bool NAME_IS_SET = false;
#define MAX_DELAY 5 // to change

#define MDNS_PORT_NUM 5353
#define SSH_PORT_NUM 22
#define BUFFER_SIZE   1500
#define MAX_LINES 24
#define HOSTNAME_SIZE 100

#define ICMP_ID 0x13
#define RESPONSE_FLAG 0x8400

#define MY_INDEX "\0x34\0x71\0x71"

#define deb(a) a

//~ vector<string> my_name;
string my_name;
uint32_t my_address; // BE order
string my_address_str;

boost::asio::io_service* io_service;


enum dns_type {
	A = 1,
	PTR = 12
};

enum service {
	UDP,
	TCP,
	NONE
};


service which_my_service(vector<string>& fqdn, size_t start) {
	if (start + 2 < fqdn.size() &&
		fqdn[start] == "_opoznienia" &&
		fqdn[start + 1] == "_udp" &&
		fqdn[start + 2] == "_local")
		return service::UDP;
	if (ssh_service && start + 2 < fqdn.size() &&
		fqdn[start] == "_ssh" &&
		fqdn[start + 1] == "_tcp" &&
		fqdn[start + 2] == "_local")
		return service::TCP;
	return NONE;
}


#endif

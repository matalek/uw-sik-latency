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
double exploration_time = 1.; // configured by -T option
double ui_refresh_time = 1; // configured by -v option
bool ssh_service; // configured by -s option

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

struct mdns_header {
	uint16_t id;
	uint16_t flags;
	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
};

struct ipv4_address {
	uint32_t ttl;
	uint32_t address; // host order
};

mdns_header read_mdns_header(char buffer[], size_t& end) {
	mdns_header res;

	uint16_t val;
	
	memcpy((char *)&val, buffer, 2);
	res.id = ntohs(val);
	memcpy((char *)&val, buffer + 2, 2);
	res.flags = ntohs(val);
	memcpy((char *)&val, buffer + 4, 2);
	res.qdcount = ntohs(val);
	memcpy((char *)&val, buffer + 6, 2);
	res.ancount = ntohs(val);
	memcpy((char *)&val, buffer + 8, 2);
	res.nscount = ntohs(val);
	memcpy((char *)&val, buffer + 10, 2);
	res.arcount = ntohs(val);

	end = 12;
	return res;
}



vector<string> read_fqdn(char buffer[], uint16_t& type_, uint16_t& class_, size_t& start) {
	vector<string> res;

	while (true) {
		uint8_t len;
		memcpy((char *)&len, buffer + start, 1);

		if (!len)
			break;
		char name[len + 1];
		memcpy(name, buffer + start + 1, len);
		name[len] = 0;
		start += len + 1;
		res.push_back(name);
		deb(std::cout << "imie: " << name << "\n";)
	}
	start++;
	memcpy((char *)&type_, buffer + start, 2);
	type_ = ntohs(type_);
	start += 2;
	memcpy((char *)&class_, buffer + start, 2);
	class_ = ntohs(class_);
	start += 2;
	
	return res;
}

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


ipv4_address read_ipv4_address(char buffer[], size_t& end) {
	ipv4_address res;
	uint32_t val;
	
	memcpy((char *)&val, buffer + end, 4);
	res.ttl = ntohl(val);
	// we ommit length - in IPv4 it is known 
	memcpy((char *)&val, buffer + end + 6, 4);
	res.address = ntohl(val);

	end += 10;
	return res;
}

#endif

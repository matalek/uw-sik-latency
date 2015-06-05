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

extern uint16_t udp_port_num; // configured by -u option
extern uint16_t ui_port_num; // configured by -U option
extern double measurement_time; // configured by -t option
extern double exploration_time; // configured by -T option
extern double ui_refresh_time; // configured by -v option
extern bool ssh_service; // configured by -s option

extern bool NAME_IS_SET;
#define MAX_DELAY 5 // to change

#define MDNS_PORT_NUM 5353
#define SSH_PORT_NUM 22
#define BUFFER_SIZE   1500
#define MAX_LINES 24
#define HOSTNAME_SIZE 100

#define ICMP_ID 0x13
#define RESPONSE_FLAG 0x8400

#define MY_INDEX "\0x34\0x71\0x71"

#define deb(a)
#define deb2(a) a

//~ vector<string> my_name;
extern string my_name;
extern uint32_t my_address; // BE order
extern string my_address_str;

extern boost::asio::io_service* io_service;
extern udp::socket* socket_mdns;

enum dns_type {
	A = 1,
	PTR = 12
};

enum service {
	UDP,
	TCP,
	NONE
};


extern service which_my_service(vector<string>& fqdn, size_t start);


#endif

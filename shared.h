#ifndef SHARED_H
#define SHARED_H

#include <iostream>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <string.h>

using namespace std;

#define MDNS_PORT_NUM 5353
#define BUFFER_SIZE   1000
#define MAX_LINES 24

#define deb(a) a

enum dns_type {
	A = 1,
	PTR = 12
};

struct mdns_header {
	uint16_t id;
	uint16_t flags;
	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
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

vector<string> read_fqdn(char buffer[], size_t& start) {
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
	return res;
}

#endif

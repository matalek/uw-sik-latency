#ifndef SHARED_H
#define SHARED_H

#include <iostream>
#include <sstream>
#include <vector>

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

mdns_header read_mdns_header(std::istringstream& iss) {
	mdns_header res;
	iss >> res.id >> res.flags >> res.qdcount >> res.ancount >> res.nscount >> res.arcount;
	return res;
}

vector<string> read_fqdn(std::istringstream& iss) {
	vector<string> res;
	while (true) {
		uint8_t len;
		iss >> len;
		if (!len)
			break;
		char name[len];
		iss.get(name, len);
		res.push_back(name);
	}
	return res;
}

#endif

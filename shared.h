#ifndef SHARED_H
#define SHARED_H

#define MDNS_PORT_NUM 5353
#define BUFFER_SIZE   1000

#define deb(a) a

enum dns_type {
	A = 1,
	PTR = 12
};

#endif

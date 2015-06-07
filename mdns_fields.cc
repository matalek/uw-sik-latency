#include "mdns_fields.h"

vector<string> read_name(char buffer[], size_t& start, size_t size) {
	vector<string> res;

	while (true) {
		if (start >= size) throw too_small_exception{};
		uint8_t len = buffer[start++];
		if (!len)
			break;

		char name[len + 1]; // +1 for easier converting to string
		if (start + len > size) throw too_small_exception{};
		if (!memcpy(name, buffer + start, len)) syserr("memcpy");
		name[len] = 0;
		start += len;
		res.push_back(name);
	}
	
	return res;
}

vector<string> read_compressable_name(char buffer[], size_t& start, size_t size) {
	vector<string> res;

	size_t real_end = 0;
	size_t real_start = start;
	bool compression_occured;
	
	while (true) {
		if (start >= size) throw too_small_exception{};
		uint8_t len = buffer[start++]; // TO DO: add checking error

		if (!len) // end of name
			break;
		
		// if two first bits are set, we encountered compression
		if ((len & 0xC0) ==  0xC0) {
			compression_occured = true;
			real_end = start + 2;
			start = ((len & 0x3F) << 8) + buffer[start];
			// considering our specific assumptions and a fact, that
			// we can only receive one type od compressed message:
			// response to PTR with fqdn: name._service._protocol.local,
			// we only accept pointer to qname
			if (!(12 <= start && start < real_start - 10)) throw bad_compression_exception{}; 
			continue;
		}

		char name[len + 1];
		if (start + len >= size) throw too_small_exception{};
		if (!memcpy(name, buffer + start, len)) syserr("memcpy");
		name[len] = 0;
		start += len;
		res.push_back(name);
	}

	if (compression_occured)
		start = real_end;
	
	return res;
}

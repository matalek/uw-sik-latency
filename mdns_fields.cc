#include "mdns_fields.h"

vector<string> read_name(char buffer[], size_t& start) {
	vector<string> res;

	while (true) {
		uint8_t len = buffer[start++];
		if (!len)
			break;

		char name[len + 1];
		memcpy(name, buffer + start, len); // TO DO: add checking error
		name[len] = 0;
		start += len;
		res.push_back(name);
		deb(std::cout << "imie: " << name << "\n";)
	}
	
	return res;
}

vector<string> read_compressable_name(char buffer[], size_t& start) {
	vector<string> res;

	size_t real_end = 0;
	bool compression_occured;
	
	while (true) {
		uint8_t len = buffer[start++]; // TO DO: add checking error

		if (!len) // end of name
			break;
		
		// if two first bits are set, we encountered compression
		if ((len & 0xC0) ==  0xC0) {
			compression_occured = true;
			real_end = start + 2;
			start = ((len & 0x3F) << 8) + buffer[start + 1];
			continue;
		}

		char name[len + 1];
		memcpy(name, buffer + start, len); // TO DO: add checking error
		name[len] = 0;
		start += len;
		res.push_back(name);
		deb4(std::cout << "imie: " << name << "\n";)
	}

	if (compression_occured)
		start = real_end;
	
	return res;
}

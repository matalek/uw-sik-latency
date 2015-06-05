#include "mdns_fields.h"

vector<string> read_name(char buffer[], size_t& start) {
	vector<string> res;

	while (true) {
		uint8_t len = buffer[start++];
		
		if (!len)
			break;
		char name[len + 1];
		memcpy(name, buffer + start, len);
		name[len] = 0;
		start += len;
		res.push_back(name);
		deb(std::cout << "imie: " << name << "\n";)
	}
	
	return res;
}

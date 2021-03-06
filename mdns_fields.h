#ifndef MDNS_FIELDS_H
#define MDNS_FIELDS_H

#include "shared.h"

class too_small_exception : exception {
	virtual const char* what() const throw() {
		return "Too small mDNS message";
	}
};

class bad_compression_exception : exception {
	virtual const char* what() const throw() {
		return "Wrongly compressed message";
	}
};

class mdns_header {

	public:

		mdns_header() { std::fill(rep_, rep_ + sizeof(rep_), 0); };

		uint16_t id() const { return decode(0, 1); }
		uint16_t flags() const { return decode(2, 3); }
		bool QR() const { return (rep_[2] & 0xA0); }
		uint16_t qdcount() const { return decode(4, 5); }
		uint16_t ancount() const { return decode(6, 7); }
		uint16_t nscount() const { return decode(8, 9); }
		uint16_t arcount() const { return decode(10, 11); }

		void id(uint16_t n) { return encode(0, 1, n); }
		void flags(uint16_t n) { return encode(2, 3, n); }
		void qdcount(uint16_t n) { return encode(4, 5, n); }
		void ancount(uint16_t n) { return encode(6, 7, n); }
		void nscount(uint16_t n) { return encode(8, 9, n); }
		void arcount(uint16_t n) { return encode(10, 11, n); }

		void read(char buffer[]) {
			if (!memcpy(rep_, buffer, 12)) syserr("memcpy");
		}

		friend std::ostream& operator<<(std::ostream& os, const mdns_header& header) {
			return os.write(reinterpret_cast<const char*>(header.rep_), 12);
		}
		

	private:
		uint16_t decode(int a, int b) const {
			return (rep_[a] << 8) + rep_[b];
		}

		void encode(int a, int b, uint16_t n) {
			rep_[a] = static_cast<uint8_t>(n >> 8);
			rep_[b] = static_cast<uint8_t>(n & 0xFF);
		}
		
		uint8_t rep_[12];
};

class mdns_answer {

	public:

		mdns_answer() { std::fill(rep_, rep_ + sizeof(rep_), 0); };

		uint16_t type() const { return decode(0, 1); }
		uint16_t class_() const { return decode(2, 3); }
		uint32_t ttl() const { return decode(4, 5, 6, 7); }
		uint16_t length() const { return decode(8, 9); }

		void type(uint16_t n) { return encode(0, 1, n); }
		void class_(uint16_t n) { return encode(2, 3, n); }
		void ttl(uint32_t n) { return encode(4, 5, 6, 7, n); }
		void length(uint16_t n) { return encode(8, 9, n); }
		
		friend std::ostream& operator<<(std::ostream& os, const mdns_answer & answer) {
			return os.write(reinterpret_cast<const char*>(answer.rep_), 10);
		}

		void read(char buffer[], size_t& start, size_t size) {
			if (start + 10 > size) throw too_small_exception{};
			if (!memcpy(rep_, buffer + start, 10)) syserr("memcpy");
			start += 10;
		}

	private:
		uint16_t decode(int a, int b) const {
			return (rep_[a] << 8) + rep_[b];
		}

		uint32_t decode(int a, int b, int c, int d) const {
			return (rep_[a] << 24) + (rep_[b] << 16) + (rep_[c] << 8) + rep_[d];
		}

		void encode(int a, int b, uint16_t n) {
			rep_[a] = static_cast<uint8_t>(n >> 8);
			rep_[b] = static_cast<uint8_t>(n & 0xFF);
		}

		void encode(int a, int b, int c, int d, uint32_t n) {
			rep_[a] = static_cast<uint8_t>(n >> 24);
			rep_[b] = static_cast<uint8_t>((n >> 16) & 0xFF);
			rep_[c] = static_cast<uint8_t>((n >> 8) & 0xFF);
			rep_[d] = static_cast<uint8_t>(n & 0xFF);
		}
		
		uint8_t rep_[10];
};

class mdns_query_end {

	public:

		mdns_query_end() { std::fill(rep_, rep_ + sizeof(rep_), 0); };

		uint16_t type() const { return decode(0, 1); }
		uint16_t class_() const { return decode(2, 3); }

		void type(uint16_t n) { return encode(0, 1, n); }
		void class_(uint16_t n) { return encode(2, 3, n); }

		friend std::ostream& operator<<(std::ostream& os, const mdns_query_end & end) {
			return os.write(reinterpret_cast<const char*>(end.rep_), 4);
		}

		void read(char buffer[], size_t& start, size_t size) {
			if (start + 4 > size) throw too_small_exception{};
			if (!memcpy(rep_, buffer + start, 4)) syserr("memcpy");
			start += 4;
		}

	private:
		uint16_t decode(int a, int b) const {
			return (rep_[a] << 8) + rep_[b];
		}

		void encode(int a, int b, uint16_t n) {
			rep_[a] = static_cast<uint8_t>(n >> 8);
			rep_[b] = static_cast<uint8_t>(n & 0xFF);
		}
		
		uint8_t rep_[4];
};

class ipv4_address {
	
	public:

		ipv4_address() { std::fill(rep_, rep_ + sizeof(rep_), 0); };

		uint32_t address() const { return decode(0, 1, 2, 3); }

		void address(uint32_t n) { return encode(0, 1, 2, 3, n); }
		
		friend std::ostream& operator<<(std::ostream& os, const ipv4_address & add) {
			return os.write(reinterpret_cast<const char*>(add.rep_), 4);
		}

		void read(char buffer[], size_t& start, size_t size) {
			if (start + 4 > size) throw too_small_exception{};
			if (!memcpy(rep_, buffer + start, 4)) syserr("memcpy");
			start += 4;
		}

	private:

		uint32_t decode(int a, int b, int c, int d) const {
			return (rep_[a] << 24) + (rep_[b] << 16) + (rep_[c] << 8) + rep_[d];
		}

		void encode(int a, int b, int c, int d, uint32_t n) {
			rep_[a] = static_cast<uint8_t>(n >> 24);
			rep_[b] = static_cast<uint8_t>((n >> 16) & 0xFF);
			rep_[c] = static_cast<uint8_t>((n >> 8) & 0xFF);
			rep_[d] = static_cast<uint8_t>(n & 0xFF);
		}
		
		uint8_t rep_[4];
};

extern vector<string> read_name(char buffer[], size_t& start, size_t size);
extern vector<string> read_compressable_name(char buffer[], size_t& start, size_t size);

#endif

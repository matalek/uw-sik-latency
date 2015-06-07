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

		unsigned short id() const { return decode(0, 1); }
		unsigned short flags() const { return decode(2, 3); }
		bool QR() const { return (rep_[2] & 0xA0); }
		unsigned short qdcount() const { return decode(4, 5); }
		unsigned short ancount() const { return decode(6, 7); }
		unsigned short nscount() const { return decode(8, 9); }
		unsigned short arcount() const { return decode(10, 11); }

		void id(unsigned short n) { return encode(0, 1, n); }
		void flags(unsigned short n) { return encode(2, 3, n); }
		void qdcount(unsigned short n) { return encode(4, 5, n); }
		void ancount(unsigned short n) { return encode(6, 7, n); }
		void nscount(unsigned short n) { return encode(8, 9, n); }
		void arcount(unsigned short n) { return encode(10, 11, n); }

		void read(char buffer[]) {
			if (!memcpy(rep_, buffer, 12)) syserr("memcpy");
		}

		friend std::ostream& operator<<(std::ostream& os, const mdns_header& header) {
			return os.write(reinterpret_cast<const char*>(header.rep_), 12);
		}
		

	private:
		unsigned short decode(int a, int b) const {
			return (rep_[a] << 8) + rep_[b];
		}

		void encode(int a, int b, unsigned short n) {
			rep_[a] = static_cast<unsigned char>(n >> 8);
			rep_[b] = static_cast<unsigned char>(n & 0xFF);
		}
		
		unsigned char rep_[12];
};

class mdns_answer {

	public:

		mdns_answer() { std::fill(rep_, rep_ + sizeof(rep_), 0); };

		unsigned short type() const { return decode(0, 1); }
		unsigned short class_() const { return decode(2, 3); }
		unsigned long ttl() const { return decode(4, 5, 6, 7); }
		unsigned short length() const { return decode(8, 9); }

		void type(unsigned short n) { return encode(0, 1, n); }
		void class_(unsigned short n) { return encode(2, 3, n); }
		void ttl(unsigned long n) { return encode(4, 5, 6, 7, n); }
		void length(unsigned short n) { return encode(8, 9, n); }
		
		friend std::ostream& operator<<(std::ostream& os, const mdns_answer & answer) {
			return os.write(reinterpret_cast<const char*>(answer.rep_), 10);
		}

		void read(char buffer[], size_t& start, size_t size) {
			if (start + 10 > size) throw too_small_exception{};
			if (!memcpy(rep_, buffer + start, 10)) syserr("memcpy");
			start += 10;
		}

	private:
		unsigned short decode(int a, int b) const {
			return (rep_[a] << 8) + rep_[b];
		}

		unsigned long decode(int a, int b, int c, int d) const {
			return (rep_[a] << 24) + (rep_[b] << 16) + (rep_[c] << 8) + rep_[d];
		}

		void encode(int a, int b, unsigned short n) {
			rep_[a] = static_cast<unsigned char>(n >> 8);
			rep_[b] = static_cast<unsigned char>(n & 0xFF);
		}

		void encode(int a, int b, int c, int d, unsigned long n) {
			rep_[a] = static_cast<unsigned char>(n >> 24);
			rep_[b] = static_cast<unsigned char>((n >> 16) & 0xFF);
			rep_[c] = static_cast<unsigned char>((n >> 8) & 0xFF);
			rep_[d] = static_cast<unsigned char>(n & 0xFF);
		}
		
		unsigned char rep_[10];
};

class mdns_query_end {

	public:

		mdns_query_end() { std::fill(rep_, rep_ + sizeof(rep_), 0); };

		unsigned short type() const { return decode(0, 1); }
		unsigned short class_() const { return decode(2, 3); }

		void type(unsigned short n) { return encode(0, 1, n); }
		void class_(unsigned short n) { return encode(2, 3, n); }

		friend std::ostream& operator<<(std::ostream& os, const mdns_query_end & end) {
			return os.write(reinterpret_cast<const char*>(end.rep_), 4);
		}

		void read(char buffer[], size_t& start, size_t size) {
			if (start + 4 > size) throw too_small_exception{};
			if (!memcpy(rep_, buffer + start, 4)) syserr("memcpy");
			start += 4;
		}

	private:
		unsigned short decode(int a, int b) const {
			return (rep_[a] << 8) + rep_[b];
		}

		void encode(int a, int b, unsigned short n) {
			rep_[a] = static_cast<unsigned char>(n >> 8);
			rep_[b] = static_cast<unsigned char>(n & 0xFF);
		}
		
		unsigned char rep_[4];
};

class ipv4_address {
	
	public:

		ipv4_address() { std::fill(rep_, rep_ + sizeof(rep_), 0); };

		unsigned long address() const { return decode(0, 1, 2, 3); }

		void address(unsigned long n) { return encode(0, 1, 2, 3, n); }
		
		friend std::ostream& operator<<(std::ostream& os, const ipv4_address & add) {
			return os.write(reinterpret_cast<const char*>(add.rep_), 4);
		}

		void read(char buffer[], size_t& start, size_t size) {
			if (start + 4 > size) throw too_small_exception{};
			if (!memcpy(rep_, buffer + start, 4)) syserr("memcpy");
			start += 4;
		}

	private:

		unsigned long decode(int a, int b, int c, int d) const {
			return (rep_[a] << 24) + (rep_[b] << 16) + (rep_[c] << 8) + rep_[d];
		}

		void encode(int a, int b, int c, int d, unsigned long n) {
			rep_[a] = static_cast<unsigned char>(n >> 24);
			rep_[b] = static_cast<unsigned char>((n >> 16) & 0xFF);
			rep_[c] = static_cast<unsigned char>((n >> 8) & 0xFF);
			rep_[d] = static_cast<unsigned char>(n & 0xFF);
			deb(for (size_t i = 0; i < 4; i++)
				cout << static_cast<int>(rep_[i]) << " ";
			cout << "\n"; )
		}
		
		unsigned char rep_[4];
};

extern vector<string> read_name(char buffer[], size_t& start, size_t size);
extern vector<string> read_compressable_name(char buffer[], size_t& start, size_t size);

#endif

#include "shared.h"
#include "icmp_client.h"
#include "computer.h"
#include "drop_to_nobody.h"

icmp_client::icmp_client() :
	socket_icmp(*io_service, icmp::v4()),
	sequence_number{0}  {

	drop_to_nobody();
}

void icmp_client::measure(uint32_t address) {
	// sending index number and group number
	stringstream ss;
	string body;
	vector<uint8_t> msg = {0x34, 0x71, 0x71, 5};
	for (size_t i = 0; i < msg.size(); i++)
		ss << msg[i];
	ss >> body;

	// create an ICMP header for an echo request
	icmp_header echo_request;
	echo_request.type(icmp_header::echo_request);
	echo_request.code(0);
	echo_request.identifier(ICMP_ID);
	echo_request.sequence_number(++sequence_number);
	compute_checksum(echo_request, body.begin(), body.end());
	
	// encode the request packet
	boost::asio::streambuf request_buffer;
	std::ostream os(&request_buffer);
	os << echo_request << body;

	// save time of sending in map
	icmp_start_times.insert(make_pair(sequence_number, get_time()));

	icmp::endpoint remote_icmp_endpoint{};
	remote_icmp_endpoint.address(boost::asio::ip::address_v4(address));
	// send the request
	socket_icmp.send_to(request_buffer.data(), remote_icmp_endpoint);

	start_icmp_receive();
}

void icmp_client::start_icmp_receive()
{
	// discard any data already in the buffer
	icmp_reply_buffer.consume(icmp_reply_buffer.size());

	// wait for a reply. We prepare the buffer to receive up to
	// BUFFER_SIZE of data.
	socket_icmp.async_receive(icmp_reply_buffer.prepare(BUFFER_SIZE),
	boost::bind(&icmp_client::handle_icmp_receive, this, boost::asio::placeholders::error, _2));
}

void icmp_client::handle_icmp_receive(const boost::system::error_code& error, std::size_t length) {
	if (error) {
		start_icmp_receive();
		return;
	}
	
	// the actual number of bytes received is committed to the buffer so that we
	// can extract it using a std::istream object.
	icmp_reply_buffer.commit(length);

	// decode the reply packet
	std::istream is(&icmp_reply_buffer);
	ipv4_header ipv4_hdr;
	icmp_header icmp_hdr;
	is >> ipv4_hdr;

	// check if ICMP protocol
	if (ipv4_hdr.protocol() != 1) {
		start_icmp_receive();
		return;
	}
	
	is >> icmp_hdr;

	uint32_t address = ipv4_hdr.source_address().to_ulong();

	// we can receive all ICMP packets received by the host, so we need to
	// filter out only the echo replies that match the our identifier and
	// expected sequence number
	map<uint16_t, uint64_t>::iterator it;
	if (is && icmp_hdr.type() == icmp_header::echo_reply
		&& icmp_hdr.identifier() == ICMP_ID
		&& (it = icmp_start_times.find(icmp_hdr.sequence_number())) != icmp_start_times.end()) {
		
		uint64_t res = get_time() - it->second;
		icmp_start_times.erase(it++);
		if (computers.count(address))
			computers[address]->notify_icmp(res);

	}
	start_icmp_receive();
}

uint64_t icmp_client::get_time() {
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return 1000000 * tv.tv_sec + tv.tv_usec;
}

icmp_client* icmp_client_;

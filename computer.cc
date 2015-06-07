#include "shared.h"
#include "computer.h"
#include "icmp_client.h"

map<uint32_t, boost::shared_ptr<computer> > computers;

computer::computer(uint32_t add, vector<string>& fqdn, uint32_t ttl) :
	opoznienia_service(false),
	ssh_service(false),
	leave_time_opoznienia(0),
	leave_time_ssh(0),
	socket_udp(*io_service),
	socket_tcp(*io_service),
	udp_sum{0},
	tcp_sum{0},
	icmp_sum{0} { 

	address = add;
	name = fqdn[0];
	add_service(fqdn, ttl);

	// creating sockets for measurement
	socket_udp.open(udp::v4());
	remote_udp_endpoint.address(boost::asio::ip::address_v4(address));
	remote_udp_endpoint.port(udp_port_num);
	
	remote_tcp_endpoint.address(boost::asio::ip::address_v4(address));
	remote_tcp_endpoint.port(SSH_PORT_NUM);
}

computer::~computer() {
	socket_udp.close();
	socket_tcp.close();
}

void computer::add_service(vector<string>& fqdn, uint32_t ttl) {
	if (fqdn.size() == 4 && fqdn[1] == "_opoznienia" && fqdn[2] == "_udp" && fqdn[3] == "local") {
		opoznienia_service = true;
		leave_time_opoznienia = get_time() + static_cast<uint64_t>(ttl) * 1000000;
	}
	
	if (fqdn.size() == 4 && fqdn[1] == "_ssh" && fqdn[2] == "_tcp" && fqdn[3] == "local") {
		ssh_service = true;
		leave_time_ssh = get_time() + static_cast<uint64_t>(ttl) * 1000000;
	}
}

void computer::measure() {
	if (opoznienia_service) {
		measure_udp();
		measure_icmp();
	}
	if (ssh_service)
		measure_tcp();

}

void computer::notify_icmp(uint64_t res) {
	icmp_times.push(res);
	icmp_sum += res;
	if (icmp_times.size() > 10) {
		icmp_sum -= icmp_times.front();
		icmp_times.pop();
	}
}

string computer::get_name() {
	return name;
}

uint32_t computer::get_udp_average() {
	if (udp_times.size())
		return udp_sum / udp_times.size();
	return 0; 
}

uint32_t computer::get_tcp_average() {
	if (tcp_times.size())
		return tcp_sum / tcp_times.size();
	return 0; 
}

uint32_t computer::get_icmp_average() {
	if (icmp_times.size())
		return icmp_sum / icmp_times.size();
	return 0; 
}

string computer::get_address_string() {
	return boost::asio::ip::address_v4(address).to_string();
}

bool computer::verify_ttl() {
	uint64_t time = get_time();
	if (time >= leave_time_opoznienia) {
		opoznienia_service = false;
		udp_times = queue<uint32_t>{};
		udp_sum = 0;
		icmp_times = queue<uint32_t>{};
		icmp_sum = 0;
	}
	if (time >= leave_time_ssh) { 
		ssh_service = false;
		tcp_times = queue<uint32_t>{};
		tcp_sum = 0;
	}
	return (opoznienia_service || ssh_service);
}

uint64_t computer::get_time() {
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return 1000000 * tv.tv_sec + tv.tv_usec;
}

void computer::measure_udp() {
	uint64_t time = get_time();

	// creating message
	boost::shared_ptr<vector<uint64_t> > message(new vector<uint64_t>{htobe64(time)});

	socket_udp.async_send_to(boost::asio::buffer(*message), remote_udp_endpoint,
		boost::bind(&computer::handle_send_udp, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
}

void computer::handle_send_udp(const boost::system::error_code& error,
	std::size_t /*bytes_transferred*/) {
	if (error) return;
	// reading
	socket_udp.async_receive_from(
		boost::asio::buffer(recv_buffer_), remote_udp_endpoint,
		boost::bind(&computer::handle_receive_udp, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred));
	
}

void computer::handle_receive_udp(const boost::system::error_code& error,
  std::size_t size /*bytes_transferred*/) {

	if (error || size > BUFFER_SIZE || size < MIN_UDP_SIZE) return;
	
	// calculating and saving time
	uint64_t val;
	if (!memcpy((char *)&val, recv_buffer_, 8)) syserr("memcpy");
	uint64_t start_time = be64toh(val);
	
	// time of receiving answer
	uint64_t end_time = get_time();

	// in task 1. it was necessery to write received data
	//  on error output. In this task we ommit it due to
	// transparity reasons.
	// However, it is still possibly to write this data
	// by uncommenting following lines.

	// if (!memcpy((char *)&val, recv_buffer_ + 8, 8)) syserr("memcpy");
	// uint64_t middle_time = be64toh(val);
	// cerr << start_time << " " << middle_time << "\n";

	uint64_t res = end_time - start_time;

	udp_times.push(res);
	udp_sum += res;
	if (udp_times.size() > 10) {
		udp_sum -= udp_times.front();
		udp_times.pop();
	}
}

void computer::measure_tcp() {
	boost::shared_ptr<uint64_t> start_time(new uint64_t{get_time()});
	
	// connect socket to the server
	socket_tcp.async_connect(remote_tcp_endpoint,
		boost::bind(&computer::handle_connect_tcp, shared_from_this(),
		start_time,
		boost::asio::placeholders::error));
}

void computer::handle_connect_tcp(boost::shared_ptr<uint64_t> start_time,
	const boost::system::error_code& error) {
	if (error) return;
	
	// time of receiving answer
	uint64_t end_time = get_time();

	uint64_t res = end_time - *start_time;
	
	tcp_times.push(res);
	tcp_sum += res;
	if (tcp_times.size() > 10) {
		tcp_sum -= tcp_times.front();
		tcp_times.pop();
	}
}

void computer::measure_icmp() {
	icmp_client_->measure(address);
}

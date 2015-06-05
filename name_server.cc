#include <stdlib.h>
#include <time.h>

#include "shared.h"
#include "computer.h"
#include "mdns_client.h"
#include "name_server.hpp"
#include "mdns_server.h"

mdns_client* mdns_client_;
name_server* name_server_;


name_server::name_server() : name(my_name), number(1),
	timer_(*io_service, boost::posix_time::seconds(MAX_DELAY)),
	timer_probes(*io_service) {
	send_query();
}

void name_server::notify() {
	deb(cout << "notify \n";)
	number++;
	send_query();
}

void name_server::send_query() {
	
	my_name = name;
	ostringstream ss;
	ss << number;
	if (number > 1)
		my_name += "(" + ss.str() + ")";

	deb2(cout << "wysyłam zapytanie o nazwy " << my_name << "\n";)

	// according to RFC 6762, chapter 8.1, we should wait for random
	// number of milisecond (0 - 250) before probing. Although we do
	// not implement the whole chapter, it is necessera to include
	// this random delay in implementation, so as to avoid conflicts
	// (which were experimentaly confirmed to appear without
	// implementing this feature).

	srand(time(NULL));
	uint8_t	wait_time = rand() % 251;

	timer_.expires_from_now(boost::posix_time::milliseconds(wait_time));
		timer_.async_wait(boost::bind(&name_server::send_probes, this,
		boost::asio::placeholders::error));
}

void name_server::send_probes(const boost::system::error_code &ec) {
	deb2(cout << "wysyłam próbki\n";)
	
	// we want to have unique name as far as both services
	// (opoznienia and ssh) are concerned, so we send a query for
	// each of this service
	vector<string> fqdn = {my_name, "_opoznienia", "_udp", "_local"};
	mdns_client_->send_query(dns_type::A, fqdn);

	fqdn = {my_name, "_ssh", "_tcp", "_local"};
	mdns_client_->send_query(dns_type::A, fqdn);

	auto probed_name = boost::shared_ptr<string>(new string(my_name));

	timer_.expires_from_now(boost::posix_time::seconds(MAX_DELAY));
		timer_.async_wait(boost::bind(&name_server::success, this,
		probed_name,
		boost::asio::placeholders::error));
}

void name_server::success(boost::shared_ptr<string> probed_name,
	const boost::system::error_code &ec) {
	deb2(cout << "koniec timera " << my_name << " " << *probed_name << " \n";)
	// we have to check, that maybe the time has passed, but this
	// function was invoked after receiving information, that someone
	// uses this name. Therefore, we check if this function was invoked
	// for name, that we currently probe
	if (ec != boost::asio::error::operation_aborted && *probed_name == my_name) {
		NAME_IS_SET = true;
		// according to standard, we have to anounce, that this name
		// has been taken, so no one would use it for themself
		mdns_server_->announce_name();
		deb2(cout << "ustalono nazwę: " << my_name << " " << number << "\n";)
	}
}


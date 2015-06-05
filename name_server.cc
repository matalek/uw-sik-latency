#include "shared.h"
#include "computer.h"
#include "mdns_client.h"
#include "name_server.hpp"
#include "mdns_server.h"

mdns_client* mdns_client_;
name_server* name_server_;


name_server::name_server() : name(my_name), number(1), timer_(*io_service, boost::posix_time::seconds(MAX_DELAY)) {
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

	// we want to have unique name as far as both services
	// (opoznienia and ssh) are concerned, so we send a query for
	// each of this service
	vector<string> fqdn = {my_name, "_opoznienia", "_udp", "_local"};
	mdns_client_->send_query(dns_type::A, fqdn);

	fqdn = {my_name, "_ssh", "_tcp", "_local"};
	mdns_client_->send_query(dns_type::A, fqdn);

	timer_.expires_from_now(boost::posix_time::seconds(MAX_DELAY));
		timer_.async_wait(boost::bind(&name_server::success, this,
		boost::asio::placeholders::error));
}

void name_server::success(const boost::system::error_code &ec) {
	if (ec != boost::asio::error::operation_aborted) {
		NAME_IS_SET = true;
		// according to standard, we have to anounce, that this name
		// has been taken, so no one would use it for themself
		mdns_server_->announce_name();
		deb2(cout << "ustalono nazwę: " << my_name << " " << number << "\n";)
	}
}


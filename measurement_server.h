#ifndef MEASUREMENT_SERVER_H
#define MEASUREMENT_SERVER_H

#include "shared.h"
#include "computer.h"
#include "mdns_server.h"

class measurement_server {
	public:
		measurement_server() : timer_(*io_service, boost::posix_time::seconds(measurement_time)) {
			deb(cout << "tworzę timer\n";)
			timer_.async_wait(boost::bind(&measurement_server::measure, this));
		}

	private:
	
		void measure() {
			deb(cout << "mierzę \n";)
			for (auto it = computers.begin(); it != computers.end(); it++)
				it->second->measure();
			timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(measurement_time));
			timer_.async_wait(boost::bind(&measurement_server::measure, this));
		}

		boost::asio::deadline_timer timer_;
};

#endif

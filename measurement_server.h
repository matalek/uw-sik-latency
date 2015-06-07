#ifndef MEASUREMENT_SERVER_H
#define MEASUREMENT_SERVER_H

#include "shared.h"
#include "computer.h"
#include "mdns_server.h"

class measurement_server {
	public:
		measurement_server() : timer_(*io_service, boost::posix_time::seconds(measurement_time)) {
			measure();
		}

	private:
	
		void measure() {
			for (auto it = computers.begin(); it != computers.end();) {
				if (it->second->verify_ttl()) {
					it->second->measure();
					it++;
				} else
					// delete computer, because its TTLs for services
					// have expired
					computers.erase(it++);
			}
			timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(measurement_time));
			timer_.async_wait(boost::bind(&measurement_server::measure, this));
		}

		boost::asio::deadline_timer timer_;
};

#endif

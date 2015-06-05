#ifndef NAME_SERVER_H
#define NAME_SERVER_H

#include "shared.h"
#include "computer.h"
#include "mdns_client.h"

extern mdns_client* mdns_client_;

class name_server {
	public:
		name_server();

		// notify, that this name is used by another computer
		void notify();

	private:
		void send_query();
		
		void send_probes(const boost::system::error_code &ec);

		void success(const boost::system::error_code &ec);

		// name of computer: name (number) (for number > 1)
		string name;
		int number;
		
		boost::asio::deadline_timer timer_;
		boost::asio::deadline_timer timer_probes;

};

extern name_server* name_server_;

#endif

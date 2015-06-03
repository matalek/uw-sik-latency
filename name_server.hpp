#ifndef NAME_SERVER_H
#define NAME_SERVER_H

#include "shared.h"
#include "computer.h"
#include "mdns_client.h"

mdns_client* mdns_client_;
class name_server {
	public:
		name_server() : name(my_name), number(1), timer_(*io_service, boost::posix_time::seconds(MAX_DELAY)) {
			send_query();
		}

		void notify() {
			deb(cout << "notify \n";)
			number++;
			send_query();
		}

	private:
		void send_query() {
			deb(cout << "wysyłam zapytanie o nazwy\n";)
			my_name = name;
			ostringstream ss;
			ss << number;
			if (number > 1)
				my_name += "(" + ss.str() + ")";
			
			
			vector<string> fqdn = {my_name, "_opoznienia", "_udp", "_local"};
			mdns_client_->send_query(dns_type::A, fqdn);

			fqdn = {my_name, "_ssh", "_tcp", "_local"};
			mdns_client_->send_query(dns_type::A, fqdn);

			//~ timer_.cancel();
			timer_.expires_from_now(boost::posix_time::seconds(MAX_DELAY));
			//~ if (!timer_waiting) {
				//~ timer_waiting = true;
				timer_.async_wait(boost::bind(&name_server::success, this,
				boost::asio::placeholders::error));
			//~ }

		}

		void success(const boost::system::error_code &ec) {
			if (ec != boost::asio::error::operation_aborted) {
				NAME_IS_SET = true;
				deb(cout << "ustalono nazwę: " << my_name << " " << number << "\n";)
			}
		}

		// name of computer: name (number) (for number > 1)
		string name;
		int number;
		
		boost::asio::deadline_timer timer_;

		bool timer_waiting;
};

#endif

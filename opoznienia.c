#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "err.h"

#define deb(a) a

#define BUFFER_SIZE   1000
#define QUEUE_LENGTH     5
#define DEFAULT_UI_PORT_NUM 3637 // configured by -U option
#define DEFAULT_UI_TIME 1 // configured by -v option

#define ESC_STR "\033"

int main(int argc, char *argv[])
{
	/*char buffer[BUFFER_SIZE];
	size_t len = 2;
	buffer[1] = 'b';
	buffer[0] = 'a';

	printf("read from %c socket: %zd bytes: %.*s\n", 27, len, (int) len, buffer); 

	printf("\033[2J"); */
	
  int sock, msg_sock;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;
  socklen_t client_address_len;
  
  char buffer[BUFFER_SIZE];
  ssize_t len, snd_len;

  sock = socket(PF_INET, SOCK_STREAM, 0); // creating IPv4 TCP socket
  if (sock <0)
    syserr("socket");
  // after socket() call; we should close(sock) on any execution path;
  // since all execution paths exit immediately, sock would be closed when program terminates

  server_address.sin_family = AF_INET; // IPv4
  server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
  server_address.sin_port = htons(DEFAULT_UI_PORT_NUM); // listening on port PORT_NUM

  // bind the socket to a concrete address
  if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    syserr("bind");

  // switch to listening (passive open)
  if (listen(sock, QUEUE_LENGTH) < 0)
    syserr("listen");

  deb(printf("accepting ui client connections on port %hu\n", ntohs(server_address.sin_port));)
  for (;;) {
    client_address_len = sizeof(client_address);
    // get client connection from the socket
     msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);
    if (msg_sock < 0)
      syserr("accept");
    do {
      len = read(msg_sock, buffer, sizeof(buffer));
      if (len < 0)
        syserr("reading from client socket");
      else {
				len = snprintf(buffer, 10, "\033[2J");
        //printf("read from socket: %zd bytes: %.*s\n", len, (int) len, buffer);
        snd_len = write(msg_sock, buffer, len);
        if (snd_len != len)
          syserr("writing to client socket");
      }
    } while (len > 0);
    printf("ending connection\n");
    if (close(msg_sock) < 0)
      syserr("close");
  }
  
	
  return 0;
}



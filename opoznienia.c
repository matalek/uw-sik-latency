#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <endian.h>

#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>

#include "err.h"

#define deb(a) a

#define BUFFER_SIZE   1000
#define QUEUE_LENGTH     5
#define DEFAULT_UI_PORT_NUM 3637 // configured by -U option
#define DEFAULT_UI_TIME 1 // configured by -v option

#define ESC_STR "\033"

uint16_t udp_port_num = 3382; // configured by -u option
uint16_t ui_port_num = 3637; // configured by -U option

void *ui_connection(void *s_ptr) {

	
  int sock, msg_sock;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;
  socklen_t client_address_len;
  
  char buffer[BUFFER_SIZE];
  ssize_t len, snd_len;

	/*
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
	*/

	sock = *(int *)s_ptr;
	free(s_ptr);
	
  for (;;) {
    /*client_address_len = sizeof(client_address);
    // get client connection from the socket
     msg_sock = accept(sock, (struct sockaddr *) &client_address, &client_address_len);
    if (msg_sock < 0)
      syserr("accept");
    */ do {
      len = read(sock, buffer, sizeof(buffer));
      if (len < 0)
        syserr("reading from client socket");
      else {
				len = snprintf(buffer, 10, "\033[2J");
        snd_len = write(sock, buffer, len);
        if (snd_len != len)
          syserr("writing to client socket");
      }
    } while (len > 0);
    printf("ending connection\n");
    if (close(sock) < 0)
      syserr("close");
  }
  
}

void udp_delay_server(int sock) {

	struct sockaddr_in client_address;
	int flags, sflags;
	socklen_t snda_len, rcva_len;
	ssize_t len, snd_len;

	snda_len = (socklen_t) sizeof(client_address);
	
	uint64_t *mesg = (uint64_t *) malloc (sizeof(uint64_t));
	if (mesg == NULL)
		syserr("malloc");
		
	rcva_len = (socklen_t) sizeof(client_address);
	flags = 0; // we do not request anything special
	len = recvfrom(sock, mesg, sizeof(uint64_t), flags,
		(struct sockaddr *) &client_address, &rcva_len);
		
	if (len < 0)
		syserr("error on datagram from client socket");
	else {
		// calculating current time
		struct timeval tv;
		gettimeofday(&tv,NULL);
		unsigned long long time = 1000000 * tv.tv_sec + tv.tv_usec;

		// creating new message
		uint64_t new_mesg[2] = {*mesg, htobe64(time)};
		len = sizeof(uint64_t[2]);

		// sending message
		sflags = 0;
		snd_len = sendto(sock, new_mesg, len, sflags,
			(struct sockaddr *) &client_address, snda_len);
				
		if (snd_len != len)
			syserr("error on sending datagram to client socket");
	}
}


#define TRUE 1
#define FALSE 0
#define BUF_SIZE 1024

static int finish = FALSE;

/* Handling end signal */
static void catch_int (int sig) {
  finish = TRUE;
  fprintf(stderr,
          "Signal %d catched. No new connections will be accepted.\n", sig);
}

/* void check_argument_presence(int i, int argc, char program[], char option) {
	if (i == argc - 1)
		fatal("%s: option requires an argument -- '%c'\n", program, option);
} */

#define check_argument_presence(option) if (i == argc - 1) \
	fatal("%s: option requires an argument -- '%c'\n", argv[0], option);

int main (int argc, char *argv[]) {
	int i;

	// parsing arguments
	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-u")) {
			check_argument_presence('u'); //(i, argc, argv[0], 'u');
			udp_port_num = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-U")) {
			check_argument_presence('U'); //(i, argc, argv[0], 'U');
			ui_port_num	= atoi(argv[++i]);
		}
	}
	
  struct pollfd client[_POSIX_OPEN_MAX];
  struct sockaddr_in server;
  char buf[BUF_SIZE];
  size_t length;
  ssize_t rval;
  int msgsock, activeClients, ret;

  // end program after Ctrl+C
  if (signal(SIGINT, catch_int) == SIG_ERR) {
    perror("Unable to change signal handler\n");
    exit(EXIT_FAILURE);
  }

  /* Inicjujemy tablicę z gniazdkami klientów, client[0] to gniazdko centrali */
  for (i = 0; i < _POSIX_OPEN_MAX; ++i) {
    client[i].fd = -1;
    client[i].events = POLLIN;
    client[i].revents = 0;
  }
  activeClients = 0;

  /* Tworzymy gniazdko centrali */
  client[0].fd = socket(PF_INET, SOCK_STREAM, 0);
  if (client[0].fd < 0) {
    perror("Opening stream socket");
    exit(EXIT_FAILURE);
  }

  /* Co do adresu nie jesteśmy wybredni */
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(ui_port_num);
  if (bind(client[0].fd, (struct sockaddr*)&server,
           (socklen_t)sizeof(server)) < 0) {
    perror("Binding stream socket");
    exit(EXIT_FAILURE);
  }

  // create socket for UDP delay server
  client[1].fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (client[1].fd < 0) {
    perror("Opening stream socket");
    exit(EXIT_FAILURE);
  }

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(udp_port_num);
  if (bind(client[1].fd, (struct sockaddr*)&server,
           (socklen_t)sizeof(server)) < 0) {
    perror("Binding stream socket");
    exit(EXIT_FAILURE);
  }

  /* Zapraszamy klientów */
  if (listen(client[0].fd, 5) == -1) {
    perror("Starting to listen");
    exit(EXIT_FAILURE);
  }

  /* Do pracy */
  do {
    for (i = 0; i < _POSIX_OPEN_MAX; ++i)
      client[i].revents = 0;
    if (finish == TRUE && client[0].fd >= 0) {
      if (close(client[0].fd) < 0)
        perror("close");
      client[0].fd = -1;
    }
		
    /* Czekamy przez 5000 ms */
    ret = poll(client, _POSIX_OPEN_MAX, 5000);
    if (ret < 0)
      perror("poll");
    else if (ret > 0) {
			
			// new telnet connection
			if (finish == FALSE && (client[0].revents & POLLIN)) {
				msgsock = accept(client[0].fd, (struct sockaddr*)0, (socklen_t*)0);
				if (msgsock == -1)
					perror("accept"); //exit(EXIT_FAILURE); ???
				else {

					int *con;
					pthread_t t;

					// only for this thread
					con = malloc(sizeof(int));
					if (!con) {
						perror("malloc");
						exit(EXIT_FAILURE);
					}
					*con = msgsock;

					if (pthread_create(&t, 0, ui_connection, con) == -1) {
						perror("pthread_create");
						exit(EXIT_FAILURE);
					}
					
					// do not wait for this thread
					if (pthread_detach(t) == -1) {
						perror("pthread_detach");
						exit(EXIT_FAILURE);
					}

					/*
					for (i = 2; i < _POSIX_OPEN_MAX; ++i) {
						if (client[i].fd == -1) {
							client[i].fd = msgsock;
							activeClients += 1;
							break;
						}
					}
					if (i >= _POSIX_OPEN_MAX) {
						fprintf(stderr, "Too many clients\n");
						if (close(msgsock) < 0)
							perror("close");
					} */
				} 
			}

			// query for UDP delay server
      if (finish == FALSE && (client[1].revents & POLLIN)) {
				udp_delay_server(client[1].fd);
      } 

      
      for (i = 2; i < _POSIX_OPEN_MAX; ++i) {
        if (client[i].fd != -1
            && (client[i].revents & (POLLIN | POLLERR))) {
          rval = read(client[i].fd, buf, BUF_SIZE);
          if (rval < 0) {
            perror("Reading stream message");
            if (close(client[i].fd) < 0)
              perror("close");
            client[i].fd = -1;
            activeClients -= 1;
          }
          else if (rval == 0) {
            fprintf(stderr, "Ending connection\n");
            if (close(client[i].fd) < 0)
              perror("close");
            client[i].fd = -1;
            activeClients -= 1;
          }
          else
            printf("-->%.*s\n", (int)rval, buf);
        }
      }
    }
    else
      fprintf(stderr, "Do something else\n");
  } while (finish == FALSE || activeClients > 0);

  if (client[0].fd >= 0)
    if (close(client[0].fd) < 0)
      perror("Closing main socket");
  exit(EXIT_SUCCESS);
}




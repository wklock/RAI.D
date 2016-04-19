/*
 * network.c
 *
 * Handle all network based functionality found in client.c and server.c
 */

#include "network.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <getopt.h>

// networking includes

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/ioctl.h>


/*
int get_socket(char *IP) {

	// ret val of getaddrinfo
	int err;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	struct addrinfo hints;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	// port in private range [49152, 65535]
	if((err = getaddrinfo(IP, "50000", &hints, &addr_info)) != 0) {
		fprintf(stderr, "IP/port error: %s\n", gai_strerror(err));
		exit(1);
	}

	if((err = connect(sock, addr_info->ai_addr, addr_info->ai_addrlen)) != 0) {
		fprintf(stderr, "connect error: %s\n", gai_strerror(err));
		exit(1);
	}

	return sock;
}
 */


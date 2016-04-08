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
#include <netdb.h>

#include "client.h"

const char usage[] = "\n\t./client (-w FILENAME or -r FILENAME) -i IP_ADDRESS -u USER\n\n";

unsigned READ;
unsigned WRITE;

char *IP;
char *USER;
char *PASSWORD;

char *FILENAME;

struct addrinfo hints, *addr_info;

void read_file(int socket) {
	
}

void write_file(int socket) {

}

void connect_controller(void) {

	// ret val of getaddrinfo
	int err;
	int sock = socket(AF_INET, SOCK_STREAM, 0);

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

}

int main(int argc, char **argv) {

	int c;

	if(argc < 4) {
		fprintf(stderr, "%s", usage);
		exit(1);
	}

	/*
	 * Usage:
	 * ./client (-w FILENAME or -r FILENAME) -i IP_ADDRESS -u USER
	 */

	while((c = getopt(argc, argv, "w:r:i:u:")) != -1) {
		switch(c) {
			case 'w':
				if(READ) { fprintf(stderr, "Cannot read and write at the same time\n"); exit(1); }
				WRITE = 1;
				FILENAME = strdup(optarg);
				break;
			case 'r':
				if(WRITE) { fprintf(stderr, "Cannot read and write at the same time\n"); exit(1); }
				READ = 1;
				FILENAME = strdup(optarg);
				break;
			case 'i':
				IP = strdup(optarg);
				break;
			case 'u':
				USER = strdup(optarg);
				break;
		}
	}

	exit(0);
}

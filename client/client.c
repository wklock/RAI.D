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

#include "client.h"

unsigned READ;
unsigned WRITE;

char *IP;
char *USER;
char *PASSWORD;

char *usage = "\n\t./client (-w FILENAME or -r FILENAME) -i IP_ADDRESS -u USER\n\n";

int main(int argc, char **argv) {

	int c;

	if(argc < 4) {
		fprintf(stderr, "%s", usage);
		exit(1);
	}

	/*
	 * Format:
	 * ./client (-w FILENAME or -r FILENAME) -i IP_ADDRESS -u USER
	 */

	while((c = getopt(argc, argv, "w:r:i:u:")) != -1) {
		switch(c) {
			case 'w':
				if(READ) { fprintf(stderr, "Cannot read and write at the same time\n"); exit(1); }
				WRITE = 1;
				break;
			case 'r':
				if(WRITE) { fprintf(stderr, "Cannot read and write at the same time\n"); exit(1); }
				READ = 1;
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

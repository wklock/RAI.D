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

size_t IP;
char *USER;
char *PASSWORD;

char *usage = "\n\t./client (-w FILENAME or -r FILENAME) -i IP_ADDRESS -u USER\n\n";

void parse_ip(char *ip) {

	/*
	 * char val[4] - current value stored character by character (to be converted using atoi)
	 * int byte - current byte, starting from top (4th byte) as its read in
	 */

	char val[4] = {0};
	int byte = 3;

	int pos = 0;

	while(byte > -1) {
		if(*ip == '.' || *ip == '\0') {

			val[pos] = '\0';
			size_t val_converted = (atoi(val) << 8*byte);
			IP += val_converted;

			pos = 0;
			byte--;

		} else {
			val[pos++] = *ip;
		}
		*ip++;
	}

}
	

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
				parse_ip(strdup(optarg));
				break;
			case 'u':
				USER = strdup(optarg);
				break;
		}
	}

	exit(0);
}

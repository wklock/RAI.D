#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <getopt.h>
#include <signal.h>

// networking includes

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "client.h"



int interrupt = 0;
const char usage[] = "\n\t./client <controller IP> <controller port>\n\n";
struct addrinfo hints, *addr_info;
static char input[2048];

void close_client() {
    interrupt = 1;
}
/*
 * Connects the client to the controller
 * Returns the file descriptor or -1 on failure
 */
int connect_to_controller(server_info_t* info) {

	// ret val of getaddrinfo
	int err;
	int sock = socket(AF_INET, SOCK_STREAM, 0);

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	// port in private range [49152, 65535]
	if((err = getaddrinfo(info->ip, info->port, &hints, &addr_info)) != 0) {
		fprintf(stderr, "IP/port error: %s\n", gai_strerror(err));
		return -1;
	}

	if((err = connect(sock, addr_info->ai_addr, addr_info->ai_addrlen)) != 0) {
		fprintf(stderr, "connect error: %s\n", gai_strerror(err));
		return -1;
	}

    return sock;

}

int main(int argc, char **argv) {

	/*
	 * Usage:
	 * ./client <controller IP> <controller port>
	 */

	if(argc != 3) {
		fprintf(stderr, "%s", usage);
		exit(1);
	}

    int sockfd;
    struct sigaction sig_act;
    sig_act.sa_handler = close_client;
    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_flags = 0;

    server_info_t* info = calloc(1, sizeof(server_info_t));
    info->ip = strdup(argv[1]);
    info->port = strdup(argv[2]);

    if((sockfd = connect_to_controller(info)) == -1) {
        fprintf(stderr, "connecting to controller failed\n");
        exit(1);
    }
    free(info);

    send(sockfd, "C", 1, 0);
    while(!interrupt) {

        fputs("RAI.D> ", stdout);

        fgets(input, 2048, stdin);
        send(sockfd, input, strnlen(input, 2048), 0);
        printf("Sent: %s", input);
    }

	exit(0);
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

// networking includes

#include <sys/socket.h>

#include "../network/network.h"



int interrupt = 0;
const char usage[] = "\n\t./client <controller IP> <controller port>\n\n";
static char input[2048];

void close_client(int i) {
    interrupt = 1;
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

    struct sigaction sig_act;
    sig_act.sa_handler = close_client;
    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_flags = 0;

    int sockfd;
    if((sockfd = get_socket(argv[1],argv[2])) == -1) {
        fprintf(stderr, "connecting to controller failed\n");
        exit(1);
    }

    // Let the server know we're a client
    send(sockfd, "C", 1, 0);

    // Set up a basic command prompt
    while(!interrupt) {

        fputs("RAI.D> ", stdout);

        fgets(input, 2048, stdin);
        send(sockfd, input, strnlen(input, 2048), 0);
        printf("Sent: %s", input);
    }

	exit(0);
}

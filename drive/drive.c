/*
** drive.c -- individual drive controller. Adapted from Beej's guide
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <signal.h>
#include "../network/network.h"


#define MAX_DATA_SIZE 100 // max number of bytes we can get at once
const char usage[] = "\n\t./drive <controller IP> <controller port>\n\n";
volatile int sockfd, interrupt = 0;


void close_client(int sig, siginfo_t *siginfo, void *context) {
    send(sockfd, "DOW", 3, 0);
    interrupt = 1;
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: drive host port\n");
        exit(1);
    }

    if((sockfd = get_socket(argv[1], argv[2])) == -1) {
        fprintf(stderr, "connecting to controller failed\n");
        exit(1);
    }

    struct sigaction sig_act;
    sig_act.sa_sigaction = close_client;
    sig_act.sa_flags = SA_SIGINFO;
    sigemptyset(&sig_act.sa_mask);
    if (sigaction(SIGINT, &sig_act, NULL) < 0) {
        perror ("sigint");
        return 1;
    }

    srand(time(NULL));
    int r = rand();
    char* file_name = malloc(sizeof("log") + sizeof(r));
    sprintf(file_name, "log:%d", r);
    FILE* log = fopen(file_name, "a+");

    ssize_t numbytes;
    char* buf = malloc(MAX_DATA_SIZE);
    char* last_message = NULL;
    // Tell the controller we are drive
    if (send(sockfd, "D", 1, 0) == -1)
        perror("send");

    while (!interrupt) {
        memset(buf, 0, MAX_DATA_SIZE);
        if ((numbytes = read(sockfd, buf, MAX_DATA_SIZE)) == -1) {
            perror("reads");
            exit(1);
        }

        if(!numbytes) {
            interrupt = 0;
            break;
        }
        if(!interrupt) {

            if (strncmp(buf, "COMMIT", numbytes) == 0) {
                printf("Received commit message\n");
                fprintf(log, "%s", last_message);
                fflush(log);
                printf("Wrote message\n");
            } else {
                printf("received %zu bytes %.*s", numbytes, numbytes, buf);
                send(sockfd, "ACK", 3, 0);
                printf("Acknowledged last message\n");
                last_message = strdup(buf);


            }
        }


    }

    close(sockfd);
    return 0;
}


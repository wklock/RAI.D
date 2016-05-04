/*
** drive.c -- individual drive controller. Adapted from Beej's guide
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>


#define PORT "25555" // the port client will be connecting to
#define MAX_DATA_SIZE 100 // max number of bytes we can get at once
char* getlocalip(int family);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);

}


int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char* buf = malloc(MAX_DATA_SIZE);
    char* last_message;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    if (argc != 2) {
        fprintf(stderr, "usage: drive host\n");
        exit(1);
    }

    srand(time(NULL));
    int r = rand();
    char* file_name = malloc(sizeof("log") + sizeof(r));
    sprintf(file_name, "log:%d", r);
    FILE* log = fopen(file_name, "a+");
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("connect");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),
              s, sizeof s);

    printf("connecting to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure
    // Tell the controller we are drive
    if (send(sockfd, "D", 1, 0) == -1)
        perror("send");

    int active = 1;
    while (active) {
        memset(buf, 0, MAX_DATA_SIZE);
        if ((numbytes = read(sockfd, buf, MAX_DATA_SIZE)) == -1) {
            perror("reads");
            exit(1);
        }

        if(!numbytes) {
            active = 0;
            break;
        }

        if(strncmp(buf, "COMMIT", numbytes) == 0) {
            printf("Received commit message\n");
            fprintf(log, "%s",last_message);
            fflush(log);
            printf("Wrote message\n");
        } else {
            printf("received %d bytes %.*s", numbytes, numbytes, buf);
            send(sockfd, "ACK", 3, 0);
            printf("Acknowledged last message\n");
            last_message = strdup(buf);
        }



    }

    close(sockfd);
    return 0;
}


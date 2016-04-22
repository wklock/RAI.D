/*
** controller.c -- stream server adapted from Beej's Guide
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10   // how many pending connections queue will hold
#define MAXDATASIZE 100

// TODO: Make sure we gracefully close and don't leak memory
void close_controller() {

}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}
/*
 * Basic networking code.
 * Returns a socket file descriptor or -1 if it failed to bind
 */
int get_socket(char* port) {
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int sockfd;
    int yes = 1;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    return sockfd;

}

int main(int argc, char** argv) {
    int numbytes;
    struct sockaddr_storage their_addr; // connector's address information
    int new_fd, server_socket;
    socklen_t sin_size;
    char s[INET_ADDRSTRLEN];
    char buf[MAXDATASIZE];

    if(argc != 2) {
        fprintf(stderr, "\n\t./controller <port>\n\n");
        exit(1);
    }

    struct sigaction sig_act;
    sig_act.sa_handler = close_controller;
    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_flags = 0;

    server_socket = get_socket(argv[1]);
    if (listen(server_socket, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }


    printf("server: waiting for connections...\n");
    while (1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(server_socket, (struct sockaddr *) &their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *) &their_addr),
                  s, sizeof s);

        printf("server: got connection from %s\n", s);

        // TODO: change this to use pthreads
        if (!fork()) { // this is the child process
            if ((numbytes = recv(new_fd, buf, MAXDATASIZE - 1, 0)) == -1) {
                perror("recv");
                exit(1);
            } else {
                buf[numbytes] = '\0';
                printf("received '%s'\n", buf);
            }

            if (send(new_fd, "Hello, world!", 13, 0) == -1)
                perror("send");

            close(new_fd);
            exit(0);

        }
        close(new_fd);  // parent doesn't need this
    }
}
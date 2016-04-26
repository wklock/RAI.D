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
#include <pthread.h>
#include "controller.h"

#define BACKLOG 10  // how many pending connections queue will hold
#define MAXDATASIZE 100
#define MAX_CLIENTS 20
#define MSG_SIZE 256

int serverSocket;
int clients[MAX_CLIENTS];
int clientsConnected;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *processClient(void *arg);

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
    int client_fd;
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

    serverSocket = get_socket(argv[1]);
    int option = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));
    if (listen(serverSocket, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    pthread_t threads[MAX_CLIENTS];
    printf("server: waiting for connections...\n");
    while (1) {  // main accept() loop
        sin_size = sizeof their_addr;
        client_fd = accept(serverSocket, (struct sockaddr *) &their_addr, &sin_size);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *) &their_addr),
                  s, sizeof s);

        printf("server: got connection from %s\n", s);

        pthread_mutex_lock(&mutex);
        if(clientsConnected < MAX_CLIENTS) {
            clients[clientsConnected] = client_fd;
            pthread_attr_t attr;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pthread_create(&threads[clientsConnected], &attr, processClient, (void*) (intptr_t) clients[clientsConnected]);
            clientsConnected++;
        } else {
            shutdown(client_fd, 2);
        }
        pthread_mutex_unlock(&mutex);
        printf("Connection made: client_fd=%d\n", client_fd);
    }
}

void *processClient(void *arg) {
    int client_fd = (intptr_t)arg;
    int client_is_connected = 1;
    while (client_is_connected) {

        char buffer[MSG_SIZE];
        int len = 0;
        int num;

        // Read until client sends eof or \n is read
        while (1) {
            num = read(client_fd, buffer + len, MSG_SIZE);
            len += num;
            printf("%s\n", buffer);
            if (!num) {
                client_is_connected = 0;
                break;
            }
            if (buffer[len - 1] == '\n')
                break;
        }

        // Error or client closed the connection, so time to close this specific
        // client connection
        if (!client_is_connected) {
            printf("User %d left\n", client_fd);
            break;
        }
    }

    close(client_fd);
    pthread_mutex_lock(&mutex);
    clientsConnected--;
    pthread_mutex_unlock(&mutex);
    return NULL;
}
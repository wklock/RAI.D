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
#include <time.h>
#include "controller.h"

#define BACKLOG 10  // how many pending connections queue will hold
#define MAXDATASIZE 100
#define MAX_CONNECTIONS 120
#define MAX_CLIENTS 20
#define MAX_DRIVES 100
#define MSG_SIZE 256

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// connected drives/clients will be stored as a linked lists of Connection structs

typedef struct Connection Connection;

struct Connection {

	char type; // either 'D' or 'C'
	int socket;
	struct sockaddr_storage info;

	Connection *next;
};

// lists
Connection *drives;
Connection *clients;

size_t drivesConnected;
size_t clientsConnected;

// adds Connection to corresponding list depending on type
void add_connection(Connection *conn);
// removes Connection given pointer
void remove_connection(Connection *conn);

void *processClient(void *arg);
void *processDrive(void *arg);
int request_status(void);

int serverSocket;

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

    pthread_t client_threads[MAX_CLIENTS];
	pthread_t drive_threads[MAX_DRIVES];

    printf("server: waiting for connections...\n");

    while (1) {  // main accept() loop

        sin_size = sizeof(their_addr);
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

		// read in connection type (client/drive initially sends 'C\n' or 'D\n')
		char type[2];
		read(client_fd, type, 2);

		// allocate new connection
		Connection *conn = malloc(sizeof(Connection));
		conn->socket = client_fd;
		conn->info = their_addr;
		conn->next = NULL;

		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

		if(clientsConnected + drivesConnected == MAX_CONNECTIONS) {

			fprintf(stderr, "at connection capacity");
			shutdown(client_fd, 2);
			free(conn);
			conn = NULL;

		} else if(type[0] == 'C') { // client connected

			if(clientsConnected < MAX_CLIENTS) {
				conn->type = 'C';
				add_connection(conn);
				pthread_create(&client_threads[clientsConnected], &attr, processClient, (void*) conn);
				clientsConnected++;
			} else {
				fprintf(stderr, "at client capacity\n");
			}

		} else if(type[0] == 'D') { // drive connected

			if(drivesConnected < MAX_DRIVES) {
				conn->type = 'D';
				add_connection(conn);
				pthread_create(&drive_threads[drivesConnected], &attr, processDrive, (void *) conn);
				drivesConnected++;
			} else {
				fprintf(stderr, "at drive capacity\n");
			}

		} else {

			fprintf(stderr, "client type unspecified/invalid (D - drive, C - client)\n");
			shutdown(client_fd, 2);
			free(conn);
			conn = NULL;

		}
        pthread_mutex_unlock(&mutex);

		if(conn) {
        	printf("Connection made: client_fd=%d\n", client_fd);
		} else {
			printf("Connection failed\n");
		}
    }
}

void *processDrive(void *arg) {

	Connection *drive = (Connection *) arg;
	int socket = drive->socket;

	int drive_is_connected = 1;

	while(drive_is_connected) {

	}

    pthread_mutex_lock(&mutex);
	remove_connection(drive);
    drivesConnected--;
    pthread_mutex_unlock(&mutex);

    return NULL;

}

void *processClient(void *arg) {

	Connection *client = (Connection *) arg;
	int socket = client->socket;

    int client_is_connected = 1;

    while (client_is_connected) {

        char buffer[MSG_SIZE];
        int len = 0;
        int num;
        read(client_fd, buffer, MSG_SIZE);
        if(buffer[0] == 'C') {
            printf("New client connection\n");
		char client_type;

        } else if (buffer[0] == 'D') {
            printf("New drive connection\n");
            send(client_fd, "D", 1, 0);
        }
        // Read until client sends eof or \n is read
        while (1) {
            num = read(socket, buffer + len, MSG_SIZE);
            len += num;
            printf("%.*s", num, buffer);

            if (!num) {
                client_is_connected = 0;
                break;
            }
            if (buffer[len - 1] == '\n') {
                break;
			}
        }

		if(request_status()) {
			printf("%.*s\n", num, buffer);
		}

        // Error or client closed the connection, so time to close this specific
        // client connection
        if (!client_is_connected) {
            printf("User %d left\n", socket);
            break;
        }
    }

    pthread_mutex_lock(&mutex);
	remove_connection(client);
    clientsConnected--;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

/*
 * Sends request to connect with drives before commit. If a drive
 * takes longer than 5 seconds to respond, return 0 and exit processes
 */

// TODO
int request_status(void) {
	return 1;
}

// adds connection node to corresponding list
void add_connection(Connection *conn) {

	Connection *list;

	if(conn->type == 'C') {
		list = clients;
	} else if(conn->type == 'D') {
		list = drives;
	} else {
		fprintf(stderr, "invalid connection type given, aborting add_connection\n");
		return;
	}

	if(!list) {
		list = conn;
	}
	while(list->next) { // move to end of list
		list = list->next;
	}
	list->next = conn;

	return;
}

// removes connection node from given list based on type and socket value
// closes socket if node found
void remove_connection(Connection *conn) {

	if(!conn) {
		fprintf(stderr, "cannot remove unprovided Connection, aborting remove_connection\n");
		return;
	}

	close(conn->socket);

	if(conn == drives) {
		drives = drives->next;
	}
	if(conn == clients) {
		clients = clients->next;
	}

	if(conn->next == NULL) {
		free(conn);
		return;
	}

	Connection *temp = conn->next;
	memcpy(conn, temp, sizeof(Connection));

	if(temp == drives) {
		drives = conn;
	}
	if(temp == clients) {
		clients = conn;
	}

	free(temp);

}

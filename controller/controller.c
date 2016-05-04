/*
** controller.c -- stream server adapted from Beej's Guide
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include "../barrier.c"
#include <fcntl.h>

#define BACKLOG 10  // how many pending connections queue will hold
#define MAX_DATA_SIZE 100
#define MAX_CONNECTIONS 120
#define MAX_CLIENTS 20
#define MAX_DRIVES 100
#define MSG_SIZE 256

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t message_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ack_received_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_barrier_t commit_barrier;
pthread_cond_t new_message_cond;

volatile int ack_received = 0;
volatile int new_message_global = 0;
char curr_message[MSG_SIZE];
int message_length = 0;


// connected drives/clients will be stored as a linked lists of Connection structs
typedef struct Connection Connection;

struct Connection {

	char type; // either 'D' or 'C'
	int socket;
    int num; // Drive 1, 2, 3...
	struct sockaddr_storage info;

	Connection *next;
};

// lists
Connection *drives;
Connection *clients;

size_t drivesConnected;
size_t drives_ack_recv = 0;

size_t clientsConnected;

// adds Connection to corresponding list depending on type
void add_connection(Connection *conn);
// removes Connection given pointer
void remove_connection(Connection *conn);

void *processClient(void *arg);
void *processDrive(void *arg);
int request_status(Connection *conn);
void* commitController(void *arg);

int serverSocket;

volatile int interrupt = 0;
// TODO: Make sure we gracefully close and don't leak memory
void close_controller() {
    interrupt = 1;
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
    char buf[MAX_DATA_SIZE];

    if(argc != 2) {
        fprintf(stderr, "\n\t./controller <port>\n\n");
        exit(1);
    }

    pthread_cond_init(&new_message_cond, NULL);

    struct sigaction sig_act;
    sig_act.sa_handler = close_controller;
    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_flags = 0;

    serverSocket = get_socket(argv[1]);
    int option = 1;
    if (listen(serverSocket, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    pthread_t client_threads[MAX_CLIENTS];
	pthread_t drive_threads[MAX_DRIVES];
    pthread_t commit_thread;
    pthread_create(&commit_thread, NULL, commitController, NULL);
    printf("server: waiting for connections...\n");

    while (!interrupt) {  // main accept() loop

        sin_size = sizeof(their_addr);
        client_fd = accept(serverSocket, (struct sockaddr *) &their_addr, &sin_size);

        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);

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
                printf("New client connection\n");
				pthread_create(&client_threads[clientsConnected], &attr, processClient, (void*) conn);
				clientsConnected++;
			} else {
				fprintf(stderr, "at client capacity\n");
			}

		} else if(type[0] == 'D') { // drive connected

			if(drivesConnected < MAX_DRIVES) {
				conn->type = 'D';
                conn->num = drivesConnected + 1;
				add_connection(conn);
                printf("New drive connection\n");
                pthread_create(&drive_threads[drivesConnected], &attr, processDrive, (void *) conn);
				drivesConnected++;
				pthread_barrier_destroy(&commit_barrier);
				pthread_barrier_init(&commit_barrier, NULL, drivesConnected);
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
        	printf("Connection made with fd: %d\n", client_fd);
		} else {
			printf("Connection failed\n");
		}
    }
}

void* commitController(void *arg) {
    /*
    int all_ack_received_local;

    pthread_mutex_lock(&all_ack_received_mutex);
    while(all_ack_received_local == all_ack_received_global) {
        pthread_cond_wait(&all_ack_received_cond, &all_ack_received_mutex);
    }
    pthread_mutex_lock(&all_ack_received_mutex);
    drives_ack_recv++;
    all_ack_received_local = all_ack_received_global;

    pthread_mutex_unlock(&all_ack_received_mutex);
    printf("Received response from all drives\nSending commit message");
    all_ack_received_global = !all_ack_received_local;
    send(socket, "COMMIT", 6, 0);
     */
    return NULL;
}

void *processDrive(void *arg) {

	Connection *drive = (Connection *) arg;
	int socket = drive->socket;
    int drive_num = drive->num;

    char response[3];
	int drive_is_connected = 1;
    int new_message_local;
	while(drive_is_connected) {
		new_message_local = new_message_global;
		pthread_mutex_lock(&message_mutex);

		while (new_message_local == new_message_global) {
			pthread_cond_wait(&new_message_cond, &message_mutex);
		}

		send(socket, curr_message, message_length, 0);

		//printf("%.*s", message_length, curr_message);
		pthread_mutex_unlock(&message_mutex);

		read(socket, response, 3);
		printf("%s\n", response);
		if (strncmp(response, "DOW", 3) == 0) {
			fprintf(stderr, "Drive: %d failed\n", drive_num);
			break;
		} else {
			printf("Received response from drive %d\n", drive_num);
			//printf("%d %zu\n", ack_received, drivesConnected);

//			pthread_mutex_lock(&ack_received_mutex);
//        	ack_received++;
//        	pthread_mutex_unlock(&ack_received_mutex);
			pthread_barrier_wait(&commit_barrier);
			//printf("%d\n", request_status(drive));
			send(socket, "COMMIT", 6, 0);
			ack_received = 0;
		}



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


        // Read until client sends eof or \n is read
        while (1) {
            message_length = read(socket, curr_message, MSG_SIZE);
            pthread_mutex_lock(&message_mutex);

            new_message_global =  !new_message_global;

            printf("%.*s", message_length, curr_message);
            pthread_cond_broadcast(&new_message_cond);

            pthread_mutex_unlock(&message_mutex);

            if (!message_length) {
                client_is_connected = 0;
                break;
            }
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
 * takes longer than 5 seconds to respond, return 0
 *
 * return -1 on error
 */

// TODO
int request_status(Connection *conn) {

	int flags;
	int socket = conn->socket;
	// get status of socket (F_GETFL)
	if((flags = fcntl(socket, F_GETFL, 0)) == -1) {
		fprintf(stderr, "request_status: invalid socket\n");
		return -1;
	}
	// if socket is good, set status flags to non-blocking (F_SETFL)
	if(fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1) {
		fprintf(stderr, "request_status: could not set nonblock status on socket\n"); 
		return -1;
	}

	// casting sockaddr_storage to sockaddr... possible issue?
	errno = 0;
	int res = connect(socket, (struct sockaddr *) &conn->info, sizeof(conn->info));

	fd_set set;
	struct timeval tv;
	int opt;
	socklen_t len = sizeof(int);

	if(res < 0) {
		if(errno == EINPROGRESS) {
			tv.tv_sec = 5;
			tv.tv_usec = 0;

			FD_ZERO(&set);
			FD_SET(socket, &set);

			if(select(socket+1, NULL, &set, NULL, &tv) > 0) {
				getsockopt(socket, SOL_SOCKET, SO_ERROR, (void *)(&opt), &len);
				if(opt) {
					fprintf(stderr, "request_status: Connection error\n");
				}
			} else {
				fprintf(stderr, "request_status: CONNECTION TIMEOUT\n");
				return 0;
			}

		} else {
			fprintf(stderr, "request_status: error connecting\n");
		}
	} else {
		fprintf(stderr, "request_status: Connection error\n");
	}

	// set back to blocking
	if((flags = fcntl(socket, F_GETFL, 0)) == -1) {
		fprintf(stderr, "request_status: invalid socket\n");
		return -1;
	}
	if(fcntl(socket, F_SETFL, flags & ~O_NONBLOCK) == -1) {
		fprintf(stderr, "request_status: could not set block status on socket\n"); 
		return -1;
	}

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

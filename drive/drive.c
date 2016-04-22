/*
** drive.c -- individual drive controller. Adapted from Beej's guide
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include <ctype.h>
#include <getopt.h>

#define PORT "3490" // the port client will be connecting to
#define MAXDATASIZE 100 // max number of bytes we can get at once
char* getlocalip(int family);
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
=======

#include <string.h>
#include "drive.h"

int main( int argc, char *argv[] ) {
   int sockfd, newsockfd, portno, clilen, hd_num;
   char buffer[256];
   struct sockaddr_in serv_addr, cli_addr;
   int n, pid;

   if (argc < 3) {
	   fprintf(stderr,"usage %s <controller ip> <port>\n", argv[0]);
	   exit(0);
   }

   /* First call to socket() function */
   sockfd = socket(AF_INET, SOCK_STREAM, 0);

   if (sockfd < 0) {
	   perror("ERROR opening socket");
       exit(1);
   }

   /* Initialize socket structure */
   bzero((char *) &serv_addr, sizeof(serv_addr));
   portno = 51000;

   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);

   /* Now bind the host address using bind() call.*/
   if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
	  perror("ERROR on binding");
	  exit(1);
   }

   /* Now start listening for the clients, here
	  * process will go in sleep mode and will wait
	  * for the incoming connection
   */

   listen(sockfd,5);
   clilen = sizeof(cli_addr);

   while (1) {
	  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

	  if (newsockfd < 0) {
		 perror("ERROR on accept");
		 exit(1);
	  }

	  /* Create child process to handle communication */
	  pid = fork();

	  if (pid < 0) {
		 perror("ERROR on fork");
		 exit(1);
	  }

	  if (pid == 0) {
		 /* This is the child process */
		 close(sockfd);
		 doprocessing(newsockfd);
		 exit(0);
	  }
	  else {
		 close(newsockfd);
	  }

   } /* end of while */
>>>>>>> master
}

int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    if (argc != 2) {
        fprintf(stderr, "usage: driveinit hostname\n");
        exit(1);
    }

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
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),
              s, sizeof s);

    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure
    // Tell the controller we are drive
    if (send(sockfd, "D", 1, 0) == -1)
        perror("send");
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';
    printf("received '%s'\n", buf);
    close(sockfd);
    return 0;
}

}

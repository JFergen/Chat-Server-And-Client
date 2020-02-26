/*      
        Joseph Fergen
        11/17/2019
        Simple chat client
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <netdb.h>

int main(int argc, char *argv[])
{
        int sockfd;
        struct hostent *server;
        int portno;
        char *usrName;
        struct sockaddr_in svr_addr;
        char svr_reply[2000];
        int maxfd;	// descriptors up to maxfd-1 polled
        int nready;	// # descriptors ready
        int nread;	// returned from read
        fd_set fds;	// set of file descriptors to poll

        if (argc != 4)
        {
                fprintf(stderr, "usage: %s <svr_host> <svr_port> <user_name>\n", argv[0]);
                exit(EXIT_FAILURE);
        }

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
                perror("socket");
                exit(EXIT_FAILURE);
        }

        server = gethostbyname(argv[1]);
        if (server == NULL)
        {
                fprintf(stderr, "error: host %s not exist\n", argv[1]);
                exit(EXIT_FAILURE);
        }

        portno = atoi(argv[2]);
        usrName = argv[3]; 

        bzero((char *) &svr_addr, sizeof(svr_addr));
        svr_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&svr_addr.sin_addr.s_addr, server->h_length);
        svr_addr.sin_port = htons(portno);

        // Connect to server
        if (connect(sockfd, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) < 0) {
                perror("connect");
                exit(EXIT_FAILURE);
        }

        // Send username to server
        if (send(sockfd, usrName, strlen(usrName), 0) < 0) {
		perror("send");
		exit(EXIT_FAILURE);
	}

        printf("Welcome to the 3600 Chat Room!\n");

	maxfd = sockfd + 1;

        // now loop forever until client disconnects
        while (1)
        {
                printf(">");
		fflush(stdout);
                
		// set up polling using select
		FD_ZERO(&fds);
		FD_SET(sockfd, &fds);
		FD_SET(0, &fds);	// stdin

		nready = select(maxfd, &fds, (fd_set *) 0, (fd_set *) 0, (struct timeval *) 0);

		// message is coming from server
		if (FD_ISSET(sockfd, &fds))
		{
		        bzero(svr_reply, 2000);	// clear out svr_reply message

			if (recv(sockfd, svr_reply, 2000, 0) < 0)
			{
				perror("recv");
				exit(EXIT_FAILURE);
			}

                        if (svr_reply[0] == '\0') {
                                printf("Server has closed. Shutting down...\n");
                                close(sockfd);
                                exit(1);
                        } else {
                                printf("%s", svr_reply);
                        }
		}

		// message is coming from stdin (i.e., integer values)
		if (FD_ISSET(0, &fds))
		{
			memset(&svr_reply[0], 0, sizeof(svr_reply));

			nread = read(0, svr_reply, sizeof(svr_reply));
               
			if (send(sockfd, svr_reply, strlen(svr_reply), 0) < 0)
			{
				perror("send");
				exit(EXIT_FAILURE);
			}
		}
        }

        close(sockfd);

        return 0;
}

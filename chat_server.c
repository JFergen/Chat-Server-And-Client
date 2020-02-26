/*      
        Joseph Fergen
        11/17/2019
        Simple chat server
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/tcp.h>

// Global Variables
struct client {
        int cli_sockfd;         // File descriptor for client
        char cli_user[100];     // Username of socket
        char blocked[29][100];       // Array of blocked usernames
} clients[30];          // Global struct to hold client's information
int numClients = 0;	// Number of clients connected (Max: 30)
char newLine = '\n';
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;      // Intialize mutex lock

void *connection_handler(void *);

int main(int argc, char *argv[]) {
        int svr_sockfd;
        int svr_portno;
        struct sockaddr_in svr_addr, cli_addr;
        socklen_t clilen;
        int *new_sockfd;
        char usrName[100];      // Holds username of client. Used to check if exists already before moving on

        // No port entered in arguments
        if (argc != 2) {
                fprintf(stderr, "Please provide a port number to use\n");
                exit(EXIT_FAILURE);
        }

        svr_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (svr_sockfd < 0) {
                perror("socket");
                exit(EXIT_FAILURE);
        }

        svr_portno = atoi(argv[1]);

        bzero((char *) &svr_addr, sizeof(svr_addr));
        svr_addr.sin_family = AF_INET;
        svr_addr.sin_addr.s_addr = INADDR_ANY;
        svr_addr.sin_port = htons(svr_portno);

        if (bind(svr_sockfd, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) < 0) {
                perror("bind");
                exit(EXIT_FAILURE);
        }

        listen(svr_sockfd, 5);

        printf("Listening on port %d\n", svr_portno);
        printf("Waiting for connections...\n");

        clilen = sizeof(cli_addr);

        while (clients[numClients].cli_sockfd = accept(svr_sockfd, (struct sockaddr *) &cli_addr, &clilen)) {
                // Denies any number of connections over 30
                if(numClients == 30) {
                        char *full = "ERR: Server can not have more than 30 connections. Connection denied. Try again later.\n";

                        printf("%s", full);
                        write(clients[numClients].cli_sockfd, full, strlen(full));      // Writes the error message to the client
                        close(clients[numClients].cli_sockfd);                          // Closes the client
                        continue;
                }

                pthread_t client_thread;
                new_sockfd = malloc(sizeof(int));
                *new_sockfd = clients[numClients].cli_sockfd;

                // Get username from client
                bzero(clients[numClients].cli_user, 100); // clear out previous message
                if (recv(clients[numClients].cli_sockfd, usrName, 100, 0) < 0) {
                        fprintf(stderr, "ERR Getting Username\n");
                        fflush(stdout);
                }

                // Checking if username already exists in server
                for (int i = 0; i < numClients; i++) {
                        if (strcmp(usrName, clients[i].cli_user) == 0) {        // If username already exists, deny entry to server
                                char usrExists[100] = "ERR: User ";
                                strcat(usrExists, usrName);
                                strcat(usrExists, " already exists. Try again with a different username.\n");

                                write(clients[numClients].cli_sockfd, usrExists, sizeof(usrExists));
                                close(clients[numClients].cli_sockfd);

                                numClients--;
                                break;
                        }
                }

                strcpy(clients[numClients].cli_user, usrName);  // If userdoes does not already exist, continue

                printf("New connection, socket fd is %d, ip is: %s, port: %d\n", clients[numClients].cli_sockfd, inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);
                printf("User %s has logged in\n", clients[numClients].cli_user);

		if (pthread_create(&client_thread, NULL, connection_handler, (void *) new_sockfd) < 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}

                if (clients[numClients].cli_sockfd < 0) { 
                        perror("accept");
                        exit(EXIT_FAILURE);
                }
                numClients++;
        }

        pthread_mutex_destroy(&mutex);
        return 0;
}

void *connection_handler(void *sockfd) {
        int cli_sockfd = *(int *) sockfd;
        int nread;
        int i;
        int j;
        char client_message[2000];
        int numBlocked = 0;
        int isBlocked = 0;
        char *help = 
                "Valid commands are:\n"
                        "\tBLOCK:<euid>\n"
                        "\tLIST\n"
                        "\tQUIT\n"
                        "\tSEND:(<euid>|*):<string>\n"
                        "\tHELP\n";     // Help command output

        for (i = 0; i < numClients; i++) {
                if (clients[i].cli_sockfd == cli_sockfd) {
                        break;
                }
        }

	bzero(client_message, 2000);	// clear out previous message

        while ((nread = recv(cli_sockfd, client_message, 2000, 0)) > 0) {
                if (client_message[strlen(client_message) - 1] == '\n') {
                        client_message[strlen(client_message) - 1] = '\0';
                }

                char *token = strtok(client_message, ":");      // User's command
                isBlocked = 0;

                // Make all characters of user's command be capital letters
                j = 0;
                char ch;
                while (token[j]) {
                        ch = token[j];
                        token[j] = toupper(ch);
                        j++;
                }

                // Commands based on client's user input
                if (strcmp(token, "SEND") == 0) {               // SEND
                        token = strtok(NULL, ":");

                        if (strcmp(token, "*") == 0) {  // Send msg to all users
                                token = strtok(NULL, "\n");

                                // From:<user> message
                                char from[1000] = "From:";
                                strcat(from, clients[i].cli_user);      // Add username to From:<User>
                                strcat(from, " (Global message)\n");
                                strcat(from, token);                    // Append client's message
                                strncat(from, &newLine, 1);             // Append newline

                                // Send message to all clients
                                for (j = 0; j < numClients; j++) {
                                        if (j == i) {
                                                continue;
                                        }

                                        write(clients[j].cli_sockfd, from, sizeof(from));     // Write the user's message to everyone
                                }
                        } else {                        // Send msg to specific user
                                char *userName = token;         // Username to search for
                                token = strtok(NULL, "\n");     // Client's message

                                // Search for username
                                for (j = 0; j < numClients; j++) {
                                        if (strcmp(userName, clients[j].cli_user) == 0) {  // Username found
                                                char from[1000] = "From:";
                                                strcat(from, clients[i].cli_user);      // Add username to From:<User>
                                                strncat(from, &newLine, 1);             // Append newline to From:<User>
                                                strcat(from, token);                    // Append client's message
                                                strncat(from, &newLine, 1);             // Append newline

                                                for (int k = 0; k < 29; k++) {
                                                        if (strcmp(clients[i].cli_user, clients[j].blocked[k]) == 0) {     // User is blocked
                                                                isBlocked = 1;
                                                                char blocked[100] = "Sorry. User \'";
                                                                strcat(blocked, userName);
                                                                strcat(blocked, "\' has blocked you\n");

                                                                write(clients[i].cli_sockfd, blocked, sizeof(blocked));
                                                                break; 
                                                        }
                                                }

                                                if (!isBlocked) {
                                                        write(clients[j].cli_sockfd, from, sizeof(from));       // Write to found client
                                                        break;
                                                } else {
                                                        break;
                                                }
                                        }

                                        if (j == (numClients - 1) ) {                          // Username not found. Print message to client
                                                char notFound[100] = "Could not find user: ";
                                                strcat(notFound, userName);     // Add username to notFound message
                                                strncat(notFound, &newLine, 1); // Append newline 

                                                write(clients[i].cli_sockfd, notFound, sizeof(notFound));       // Write error message to old client
                                        }
                                }
                        }
                } else if (strcmp(token, "LIST") == 0) {        // LIST
                        char users[200];        // Holds the list of users
                        strcat(users, "List of users:\n");
                        
                        // Get list of users
                        for (j = 0; j < numClients; j++) {
                                if (strcmp(clients[j].cli_user, "") != 0) {
                                        strcat(users, clients[j].cli_user);     // Append username to list
                                        strncat(users, &newLine, 1);            // Append newline
                                }  
                        }

                        write(clients[i].cli_sockfd, users, sizeof(users));     // Write list of users to client
                } else if (strcmp(token, "QUIT") == 0) {        // QUIT
                        close(clients[i].cli_sockfd);   // Close the file descriptor of that socket
                } else if (strcmp(token, "BLOCK") == 0) {       // BLOCK
                        token = strtok(NULL, " \n");    // Get username to block

                        for (j = 0; j < numClients; j++) {
                                if (strcmp(token, clients[j].cli_user) == 0) {  // Found username to block
                                        char blocked[100] = "Done. You will not receive messages from ";
                                        strcat(blocked, token);         // Append username to blocked message
                                        strncat(blocked, &newLine, 1);  // Append newline

                                        strcpy(clients[i].blocked[numBlocked], token);
                                        write(clients[i].cli_sockfd, blocked, sizeof(blocked));
                                        numBlocked++;
                                        break;
                                }

                                if (j == (numClients - 1) ) {
                                        char notFound[100] = "Could not find user: ";
                                        strcat(notFound, token);        // Add username to notFound message
                                        strncat(notFound, &newLine, 1); // Append newline 

                                        write(clients[i].cli_sockfd, notFound, sizeof(notFound));       // Write error message to old client
                                }
                        }
                } else if (strcmp(token, "UNBLOCK") == 0) {     // UNBLOCK
                        token = strtok(NULL, " \n");    // Get username to unblock

                        for (j = 0; j < numClients; j++) {
                                if (strcmp(token, clients[j].cli_user) == 0) {  // Found username to unblock
                                        char unBlocked[100] = "Done. You will receive messages from ";
                                        strcat(unBlocked, token);         // Append username to blocked message
                                        strncat(unBlocked, &newLine, 1);  // Append newline

                                        numBlocked--;
                                        strcpy(clients[i].blocked[numBlocked], "");
                                        write(clients[i].cli_sockfd, unBlocked, sizeof(unBlocked));
                                        break;
                                }

                                if (j == (numClients - 1) ) {
                                        char notFound[100] = "Could not find user: ";
                                        strcat(notFound, token);        // Add username to notFound message
                                        strncat(notFound, &newLine, 1); // Append newline 

                                        write(clients[i].cli_sockfd, notFound, sizeof(notFound));       // Write error message to old client
                                }
                        }
                } else if (strcmp(token, "HELP") == 0) {        // HELP
                        write(clients[i].cli_sockfd, help, strlen(help) + 1);       // Writes help command to client
                } else {                                        // Command does not exist
                        char noCommand[300] = "Command \'";
                        strcat(noCommand, token);       // Append command that user entered to message
                        strcat(noCommand, "\' does not exist.\n");
                        strcat(noCommand, help);        // Append HELP command

                        write(clients[i].cli_sockfd, noCommand, sizeof(noCommand));     // Write error message to old client
                }

                fflush(stdout);
	 	bzero(client_message, 2000);	// clear out previous message
        }

	// Client connection can be reset by peer, so just disconnect cleanly
        if (nread <= 0) {
                pthread_mutex_lock(&mutex);     // Mutex lock to change global variable 
		fprintf(stderr, "User %s disconnected\n", clients[i].cli_user);
                strcpy(clients[i].cli_user, "");
                fflush(stdout);
                pthread_mutex_unlock(&mutex);   // Unlock mutex lock
        }

        free(sockfd);
        return 0;
}

//
// Created by vincents on 13.10.17.
//

/*IMPORTANT NOTES FOR UNDERSTANDING
 * socklen_t:   defined as unsigned int --> encapsulates the addrlen datatype from environment by using this layer
 * of abstraction*/

//general imports
#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close

#define TRUE   1
#define FALSE  0
#define PORT 8888

#ifdef __linux__
#define OS_Linux
#elif defined(_Win32) || defined(WIN32)
#define OS_Windows
#endif
#ifdef OS_Linux
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> //close
#include <arpa/inet.h>
#include <netdb.h>
//Macros
#define error(x) perror(x)  //error printout
#elif defined(OS_Windows)
#include <winsock2.h>
#include <ws2tcpip.h>
//Macros
#define close(x) closesocket(x)
#define error(x) printf("%s Error: %d", x, WSAGetLastError())   //error printout
#endif

int main(int argc , char *argv[]) {
    int opt = TRUE;
#ifdef OS_Windows
    //initializations for winsock
    WSADATA wsa;
    SOCKET master_socket, new_socket, client_sockets[30], sd, max_sd;
    printf("Initializing WINSock...");
    if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        error("wsainit");
        exit(EXIT_FAILURE);
    }
    printf("Initialized!\n");
#elif defined(OS_Linux)
    //initialisations for unix
    int master_socket, new_socket, client_sockets[30], sd, max_sd;
#endif
    int addrlen, max_clients = 30, activity, i, valread;
    struct sockaddr_in address;
    char buffer[1025];  //data buffer of 1KB
    fd_set readfds; // set of socket descriptors

    //initialize socket address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //welcome message
    char *himessage = "WELCOME: ECHOING CHAT SERVER V1.1 \r\n\r\n";
    char *byemessage = "CHAT SERVER SAYS GOODBYE! - Session will be closed shortly! \r\n";

    //initialise all client_sockets[] to 0 (empty)
    for (i = 0; i < max_clients; i++) {
        client_sockets[i] = 0;
    }

    //create master socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        error("socket failed");
        exit(EXIT_FAILURE);
    }

    /*stackoverflow: set master socket to allow multiple connections
     * this is just a good habit, it will work without this*/
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
        error("setsockopt");
        exit(EXIT_FAILURE);
    }

    //bind the socket to localhost port 8888
    if (bind(master_socket, (struct sockaddr *) &address, sizeof(address)) < 0) {
        error("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %d \n", PORT);

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0) {
        error("listen");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connection
    addrlen = sizeof(address);
    puts("Waiting for client connections ...");

    while (TRUE) {
        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        //add child sockets to set
        for (i = 0; i < max_clients; i++) {
            //socket descriptor
            sd = client_sockets[i];

            //if valid socket descriptor then add to read list (0 --> not valid)
            if (sd > 0)
                FD_SET(sd, &readfds);

            //highest file descriptor number, need it for the select function
            if (sd > max_sd)
                max_sd = sd;
        }

        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        //if error or system interrupt then stop and print error
        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }

        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
            if (0 > (new_socket = accept(master_socket, (struct sockaddr *) &address, (socklen_t *) & addrlen))) {
                error("accept");
                exit(EXIT_FAILURE);
            }
            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n", new_socket,
                   inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            //send new connection greeting message
            if (send(new_socket, himessage, strlen(himessage), 0) != strlen(himessage)) {
                error("send");
            }

            puts("Welcome message sent successfully");

            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++) {
                //if position is empty
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);

                    break;
                }
            }
        }

        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++) {
            sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                memset(buffer, '\0', sizeof buffer);
                //Check if it was for closing , and also read the incoming message
                if ((valread = recv(sd, buffer, 1024, 0)) == 0) {
                    //Somebody disconnected , get his details and print
                    getpeername(sd, (struct sockaddr *) &address, (socklen_t *) &addrlen);
                    printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr),
                           ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    if(close(sd) < 0) {
                        error("closingfd");
                    }
                    client_sockets[i] = 0;
                }

                    //Echo back the message that came in
                else {
                    //terminate session if client has asked for (keyword is "--exit")
                    if(strcmp(buffer, "--exit") == 0) {
                        //Somebody disconnected , get his details and print
                        getpeername(sd, (struct sockaddr *) &address, (socklen_t *) &addrlen);
                        printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr),
                               ntohs(address.sin_port));
                        if(send(sd, byemessage, strlen(byemessage), 0) != strlen(byemessage)) {
                            error("byesend");
                        }
                        //Close the socket and mark as 0 in list for reuse
                        if(close(sd)) {
                            error("closingfd");
                        }
                        client_sockets[i] = 0;
                    } else {
                        //set the string terminating NULL byte on the end of the data read
                        buffer[valread] = '\0';
                        for(int j = 0; j < max_clients; j++) {
                            sd = client_sockets[j];
                            if(sd != 0)
                                send(sd, buffer, strlen(buffer), 0);
                        }
                    }
                }
            }
        }
    }

    return 0;
}


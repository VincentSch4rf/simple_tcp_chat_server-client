//
// Created by vincents on 13.10.17.
//

#include <stdio.h> //printf
#include <string.h>    //strlen
#include <unistd.h> // close
#include <stdlib.h>

#ifdef __linux__
#define OS_Linux
#elif defined(WIN32) || defined(_WIN32)
#define OS_Windows
#endif

#ifdef OS_Linux
#include <sys/socket.h>    //socket
#include <arpa/inet.h>  //ip_address
#include <netdb.h>
#include <unitypes.h>
#include <errno.h>
//Macros
#define SOCKET int
#define error(x) perror(x)  //error printout
#elif defined(OS_Windows)
#include <winsock2.h>
#include <stdint.h>
#pragma comment(lib,"ws2_32.lib")
//Macros
#define close(x) closesocket(x) //close socket
#define error(x) printf("%s Error: %d", x, WSAGetLastError())   //error printout
#endif

#define STDIN 0

//Variables
char message[1024], buf[1024];;
SOCKET sock;
int n;
fd_set master, read_fds;

int main(int argc17, char *argv[]) {
#ifdef OS_Windows
    //initializations for winsock
    WSADATA wsa;
    printf("Initializing WINSock...");
    if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        error("wsainit");
        exit(EXIT_FAILURE);
    }
    printf("Initialized!\n");
#endif
    struct sockaddr_in server;
    int pos = 0;
    char ip_address[16], port_s[8], *server_reply;
    char *ptr;
    long port;

    //Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        error("createsocket");
    }
    puts("Socket created!");

    //get user input for host
    printf("Please enter the IP-address of the host you want to connect to:\n");
    scanf("%s", ip_address);
    printf("Please enter the port on which to connect to the server:\n");
    scanf("%s", port_s);
    port = strtol(port_s, &ptr, 10);    //Convert string to long which can later be casted to uint16_t for use in htons()
    printf("Host set!\nTrying to connect...");

    server.sin_addr.s_addr = inet_addr(ip_address);
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t) port);
    memset(server.sin_zero, '\0', sizeof server.sin_zero);

    //Connect to remote server
    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        error("connect");
        return 1;
    }
    puts("Connected!\n");

    fd_set connset, readset;
    int size, nread, msglen, bytesToRead, bytesToSend, cnt, nsent = 0;
    FD_ZERO(&connset);

    while (1) {
        FD_ZERO(&master);
        FD_ZERO(&read_fds);

        FD_SET(0, &master);
        FD_SET(sock, &master);
        while (1) {
            read_fds = master;
            if (select(sock + 1, &read_fds, NULL, NULL, NULL) == -1) {
                perror("select:");
                exit(1);
            }
            // Listen for server messages
            if (FD_ISSET(sock, &read_fds)) {
                n = read(sock, buf, sizeof(buf));
                buf[n] = 0;
                if (n < 0) {
                    error("read");
                    exit(1);
                }
                printf("Server reply: %s", buf);
            }
            // Listen for userinput
            if (FD_ISSET(STDIN, &read_fds)) {
                n = read(STDIN, message, sizeof(message));
                message[n] = 0;
                n = write(sock, message, strlen(message));
                if (n < 0) {
                    perror("writeThread:");
                    exit(1);
                }
                if (strcmp(message, "--exit")) {
                    printf("Terminating session...");
                    break;
                }
                memset(message, '\0', sizeof(message));
            }
        }
        close(sock);
#ifdef OS_Windows
        WSACleanup();
#endif
    }
}


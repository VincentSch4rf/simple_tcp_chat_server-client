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
//Macros
#define error(x) perror(x)  //error printout
#elif defined(OS_Windows)
#include <winsock2.h>
#include <stdint.h>
#pragma comment(lib,"ws2_32.lib")
//Macros
#define close(x) closesocket(x) //close socket
#define error(x) printf("%s Error: %d", x, WSAGetLastError())   //error printout
#endif

int main(int argc , char *argv[])
{
#ifdef OS_Windows
    //initializations for winsock
    WSADATA wsa;
    SOCKET sock;
    printf("Initializing WINSock...");
    if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        error("wsainit");
        exit(EXIT_FAILURE);
    }
    printf("Initialized!\n");
#elif defined(OS_Linux)
    //initialisations for unix
    int sock;
#endif
    struct sockaddr_in server;
    int pos = 0;
    char ip_address[16], port_s[8], message[1000] , server_reply[1000];
    char *ptr;
    long port;

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        error("createsocket");
    }
    puts("Socket created!");

    //get user input for host
    printf("Please enter the IP-address and port of the server you want to connect to:\n");
    scanf("%s", ip_address);
    printf("Please enter the port on which to connect to the server:\n");
    scanf("%s", port_s);
    port = strtol(port_s, &ptr, 10);    //Convert string to long which can later be casted to uint16_t for use in htons()
    printf("Host set!\nTrying to connect...");

    server.sin_addr.s_addr = inet_addr(ip_address);
    server.sin_family = AF_INET;
    server.sin_port = htons( (uint16_t) port );
    memset(server.sin_zero, '\0', sizeof server.sin_zero);

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        error("connect");
        return 1;
    }

    puts("Connected!\n");

    //Get welcome message
    if( recv(sock , server_reply , 1024 , 0) < 0)
    {
        error("recv");
    }
    puts(server_reply);

    //keep communicating with server
    while(1)
    {
        memset(server_reply, '\0', sizeof server_reply);
        printf("Enter message :\n");
        scanf("%s" , message);

        //Send some data
        if( send(sock , message , strlen(message) , 0) != strlen(message))
        {
            error("send");
            break;
        }

        //Receive a reply from the server
        if( recv(sock , server_reply , 1024 , 0) < 0)
        {
            error("recv");
            break;
        }

        puts("Server reply :");
        puts(server_reply);

        if(strcmp(message, "--exit") == 0) {
            printf("Terminating session...");
            break;
        }
    }

    if(close(sock) < 0) {
        error("closingfd");
    }
#ifdef OS_Windows
    WSACleanup();
#endif
    printf("Session terminated!");
    return 0;
}


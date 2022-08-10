#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>

#define PORT 8002
#define SIZE 1024
#define IP_ADDR "127.0.0.1"
#define SERVER_PATH "m4"

int main(int argc, char *argv[]){

    // Information UNIX Socket
    int serverSk, clientSk;
    int serverLen, clientLen;
    char inputData[SIZE];
    int unixFlag;
    struct sockaddr_un server_addr;
    struct sockaddr_un client_addr;

    // Information INET Socket
    int inetSk;
    int inetLen;
    int inetFlag;
    char inetData[SIZE];
    struct sockaddr_in inetaddr;

    // Nonblocking Message
    int unixMsg;
    int inetMsg;

    // UNIX Socket ( Monitor - Input )
    serverSk = socket(AF_UNIX, SOCK_STREAM, 0);

    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, SERVER_PATH);
    serverLen = sizeof(server_addr);

    if( 0 == access(SERVER_PATH, F_OK)){
        unlink(SERVER_PATH);
    }

    // Input(UNIX) Connect
    if (bind(serverSk, (struct sockaddr *)&server_addr, serverLen)){
        perror("bind");
        exit(1);
    }

    if (listen(serverSk, 5) < 0){
        perror("listen");
        exit(1);
    }

    clientLen = sizeof(client_addr);
    if ((clientSk = accept(serverSk, (struct sockaddr *)&client_addr, &clientLen)) == -1){
        perror("accept");
        exit(1);
    }
    // Set UNIX NonBlocking 
    unixFlag = fcntl(clientSk, F_GETFL, 0);
    fcntl(clientSk, F_SETFL, unixFlag | O_NONBLOCK);
    
    // INET Socket ( Server - Monitor )
    inetSk = socket(AF_INET, SOCK_STREAM, 0);
    inetaddr.sin_family = AF_INET;
    inetaddr.sin_addr.s_addr = inet_addr(IP_ADDR);
    inetaddr.sin_port = htons(PORT);
    // INET Connect
    
    inetLen = sizeof(inetaddr);
    if (connect(inetSk, (struct sockaddr *)&inetaddr, inetLen) == -1){
        perror("connect");
        exit(1);
    }else{
        printf("[INFO] Connected to Waiting Room\n");
    }
    
    // Set INET NonBlocking
    inetFlag = fcntl(inetSk, F_GETFL, 0);
    fcntl(inetSk, F_SETFL, inetFlag | O_NONBLOCK);

    while(1){

        // Receive Message From INPUT
        if (unixMsg = recv(clientSk, inputData, SIZE, 0) > 0){

            if(send(inetSk, inputData, SIZE, 0) == -1){
                perror("send");
                exit(1);
            }

            if (strcmp(inputData, "3") == 0){
                printf("Exit\n");
                exit(1);
            }
        }


        if (inetMsg = recv(inetSk, inputData, SIZE, 0) > 0){
            printf("%s\n", inputData);
        }







    }




}

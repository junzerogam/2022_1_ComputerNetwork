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
#define SERVER_PATH "m2"

int main(int argc, char *argv[]){

    int clientSk, clientLen;
    struct sockaddr_un input;
    char data[SIZE];

    //Create Unix Socket for input
    if ((clientSk = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(1);
    }

    memset((char *)&input, '\0', sizeof(input));
    input.sun_family = AF_UNIX;
    strcpy(input.sun_path, SERVER_PATH);
    clientLen = sizeof(input);

    if (connect(clientSk, (struct sockaddr *)&input, clientLen) < 0){
        perror("connect");
        exit(1);
    }else {
        printf("[INFO] Connected to Monitor\n");
    }

    // Input Message

    while(1){
        printf("Enter >> ");
        fgets(data, SIZE, stdin);

        if (send(clientSk, data, SIZE, 0) == -1){
            perror("send");
            exit(1);
        }

        if (strcmp(data, "3") == 0){
            exit(1);
        }

        memset(data, 0, SIZE);


    }









}


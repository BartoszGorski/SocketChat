#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

#define DOMAIN AF_INET      //komunikacja lokalna
#define TYPE SOCK_STREAM    //dwukierunkowa komunikacja polaczeniowa, strumieniowa, z kontrola poprawnosci
#define PROTOCOL 0
#define BUFFER_LENGTH sizeof(struct clientMessageDate)
#define MESSAGE_LENGTH 250
#define PORT 777
#define ADDRESS "127.0.0.1"
#define NAME_SIZE 20

struct clientMessageDate {
    char myName[NAME_SIZE];
    char destName[NAME_SIZE];
    char message[MESSAGE_LENGTH];
};


void *recive(void *server) {

    char buffer[MESSAGE_LENGTH];

    int serverId = *(int *) server;
    int recive;
    while (recive = recv(serverId, buffer, sizeof(buffer), 0)) {
        if (recive == -1) {
            perror("recv() failed\n");
        }
        printf("%s\n", buffer);

        if (recive == 0 || recive < sizeof(buffer)) {
            printf("The server closed the connection before all of the data was sent\n");
        }
    }
}

int main(int argc, char *argv[]) {

    struct sockaddr_in mySockAddr;    //struktura opisujaca adres socketu
    char buffer[BUFFER_LENGTH];
    struct clientMessageDate structClientData;

    //alokacja soketu
    int mySocket = socket(DOMAIN, TYPE, PROTOCOL);
    if (mySocket == -1) {
        perror("socket() failed\n");
        exit(EXIT_FAILURE);
    }

    mySockAddr.sin_family = DOMAIN;
    mySockAddr.sin_addr.s_addr = inet_addr(ADDRESS);
    mySockAddr.sin_port = PORT;

    int myConnection = connect(mySocket, (struct sockaddr *) &mySockAddr, sizeof(struct sockaddr_in));

    if (myConnection == -1) {
        perror("connect() failed\n");
        close(myConnection);
        exit(EXIT_FAILURE);
    }

    if (myConnection == 0) {
        printf("CONNECTION SUCCESS\n");
    }

    //sizeof(structClientData.myName)
    printf("Your name: ");
    fgets(structClientData.myName, sizeof(structClientData.myName), stdin);//read first line
    structClientData.myName[strlen(structClientData.myName) - 1] = '\0';

    //wysyla imie
    memcpy(buffer, &structClientData, BUFFER_LENGTH);
    if (send(mySocket, buffer, BUFFER_LENGTH, 0) == -1) {
        perror("send() failed\n");
    }

    pthread_t recive_thread;
    if ((pthread_create(&recive_thread, NULL, recive, (void *) &mySocket)) < 0) {
        perror("could not create thread");
        exit(EXIT_FAILURE);
    }

    sleep(1);

    while (1) {

        printf("Write message: \n");
        fgets(structClientData.message, sizeof(structClientData.message), stdin);
        structClientData.message[strlen(structClientData.message) - 1] = '\0';

        printf("Send to: ");
        fgets(structClientData.destName, sizeof(structClientData.destName), stdin);
        structClientData.destName[strlen(structClientData.destName) - 1] = '\0';

        memcpy(buffer, &structClientData, BUFFER_LENGTH);
        if (send(mySocket, buffer, BUFFER_LENGTH, 0) == -1) {
            perror("send() failed\n");
        }
    }

    close(myConnection);
    close(mySocket);

    return 0;
}

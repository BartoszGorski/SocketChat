#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <unistd.h>

#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>

#define DOMAIN AF_INET      //komunikacja lokalna
#define TYPE SOCK_STREAM    //dwukierunkowa komunikacja polaczeniowa, strumieniowa, z kontrola poprawnosci
#define BUFFER_LENGTH sizeof(struct clientMessageDate)
#define LISTEN_BACKLOG 10   //ilosc przychodzacych polaczen ktore moze czekac w kolejce (az serwer je zaakceptuje i obsluzy)
#define MAX_CLIENTS    10
#define PROTOCOL 0
#define ADDRESS "127.0.0.1"
#define PORT 777

#define MESSAGE_LENGTH 250
#define NAME_SIZE 20


struct clientMessageDate {
    char myName[NAME_SIZE];
    char destName[NAME_SIZE];
    char message[MESSAGE_LENGTH];
};

struct clientData {
    int client_id[MAX_CLIENTS];
    int indexLastConnected;
    char name[NAME_SIZE];
    struct clientMessageDate message[MAX_CLIENTS];
};

void sendback(int clientConnection, char message[]) {
    int sendBack = send(clientConnection, message, MESSAGE_LENGTH, 0);
    if (sendBack == -1) {
        perror("send() failed\n");
    }
}

void *recive(void *clientData) {

    struct clientData *clients;
    clients = (struct clientData *) clientData;

//    struct clientMessageDate reciveMessage;

    int index = clients->indexLastConnected;
    char buffer[BUFFER_LENGTH];
    int recive;

    while ((recive = recv(clients->client_id[index], buffer, BUFFER_LENGTH, 0))) {

        if (recive == -1) {
            perror("recv() failed ");
            return 0;
        }

        memcpy(&clients->message[index], buffer, BUFFER_LENGTH);

        if(!clients->name[0]){
            strncpy(clients->name,clients->message[index].myName,NAME_SIZE);

            printf("strlen(clients->name) %d\n",strlen(clients->name));
            printf("clients->message.myName %s\n",clients->message[index].myName);
            printf("clients->name %s\n",clients->name);

        }

        printf("Online:\n");
        for (int j = 0; j < MAX_CLIENTS; j++) {
            if (clients->client_id[j] != -1) {
                printf("%s\n", clients->name);
            }
        }

        printf("Server recived from '%s' message: %s\n",clients->message[index].myName, clients->message[index].message);

        if (recive == 0 || recive < BUFFER_LENGTH) {
            printf("The clients closed the connection before all of the data was sent ");
            return 0;
        }

        for (int j = 0; j < MAX_CLIENTS; j++) {
            if (clients->client_id[j] != -1 && j != index) {
                sendback(clients->client_id[j], clients->message[index].message);
            }
        }
    }

    close(clients->client_id[index]);
    clients->client_id[index] = -1;

    return 0;
}

int new_connection(int *clientConnection, int *mySocket, struct sockaddr_in *clientAddr) {

    socklen_t clientAddrSize = sizeof(struct sockaddr);
    *clientConnection = accept(*mySocket, (struct sockaddr *) clientAddr, &clientAddrSize);
    printf("New connection\n");

    if (*clientConnection == -1) {
        perror("accept() failed\n");
        close(*clientConnection);
        exit(EXIT_FAILURE);
    }

    return 1;
}

void startServer(int *mySocket, struct sockaddr_in *mySockAddr) {

    //alokacja soketu
    *mySocket = socket(DOMAIN, TYPE, PROTOCOL);
    if (*mySocket == -1) {
        perror("socket() failed ");
        close(*mySocket);
        exit(EXIT_FAILURE);
    }

    mySockAddr->sin_family = DOMAIN;
    mySockAddr->sin_addr.s_addr = inet_addr(ADDRESS);
    mySockAddr->sin_port = PORT;

    if (bind(*mySocket, (struct sockaddr *) mySockAddr, sizeof(struct sockaddr)) == -1) {
        perror("bind() failed ");
        exit(EXIT_FAILURE);
    }

    if (listen(*mySocket, LISTEN_BACKLOG) == -1) {
        perror("listen() failed ");
        exit(EXIT_FAILURE);
    }
}

int main() {


    int mySocket = -1, clientConnection = -1, *new_client;
    struct sockaddr_in mySockAddr; //struktura opisujaca adres socketu
    struct sockaddr_in clientAddr; //struktura opisujaca adres polaczenia przychodzacego

    startServer(&mySocket, &mySockAddr);
    printf("Server started\n");

    struct clientData clientInfo;
    clientInfo.indexLastConnected = 0;
//    clientInfo.name[0] = '0';

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clientInfo.client_id[i] = -1;
    }

    pthread_t recive_thread;

    while (1) {

        new_connection(&clientConnection, &mySocket, &clientAddr);

        printf("clinetConn ID %d\n", clientConnection);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientInfo.client_id[i] == -1) {
                clientInfo.client_id[i] = clientConnection;
                clientInfo.indexLastConnected = i;
                break;
            }
        }

        if ((pthread_create(&recive_thread, NULL, recive, (void *) &clientInfo)) < 0) {
            perror("could not create thread");
            exit(EXIT_FAILURE);
        }
    }

    close(mySocket);

    return 0;
}

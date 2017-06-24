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
    char name[NAME_SIZE][MAX_CLIENTS];
    struct clientMessageDate message[MAX_CLIENTS];
};

char *concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void sendback(int clientConnection, char message[]) {
    int sendBack = send(clientConnection, message, MESSAGE_LENGTH, 0);
    if (sendBack == -1) {
        perror("send() failed\n");
    }
}

void sendOnlinelist(struct clientData *clients, int index ){
    printf("-----Online-----\n");
    strcpy(clients->message[index].message,"-----Online-----\n");
    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (clients->client_id[j] != -1) {
            printf("%d %s\n",j, clients->name[j]);
            strcpy(clients->message[index].message,concat(clients->message[index].message,clients->name[j]));
            strcpy(clients->message[index].message,concat(clients->message[index].message,"\n"));
        }
    }
    printf("----------------\n");
    strcpy(clients->message[index].message, concat(clients->message[index].message,"----------------\n"));

    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (clients->client_id[j] != -1) {
            sendback(clients->client_id[j], clients->message[index].message);
        }
    }
}

void *recive(void *clientData) {

    struct clientData *clients;
    clients = (struct clientData *) clientData;

    int index = clients->indexLastConnected;
    char buffer[BUFFER_LENGTH];
    int recive;

    while ((recive = recv(clients->client_id[index], buffer, BUFFER_LENGTH, 0))) {

        if (recive == -1) {
            perror("recv() failed ");
            return 0;
        }

        if (recive == 0 || recive < BUFFER_LENGTH) {
            printf("The clients closed the connection before all of the data was sent ");
            return 0;
        }

        memcpy(&clients->message[index], buffer, BUFFER_LENGTH);

        int rc = strcmp(clients->name[index], "\0");
        if(rc == 0){

            strncpy(clients->name[index],clients->message[index].myName, strlen(clients->message[index].myName));

            sendOnlinelist(clients, index);

        } else {
            printf("Server recived from '%s' message: %s\n",clients->message[index].myName, clients->message[index].message);

            char tempBuffor[MESSAGE_LENGTH];
            strcpy(tempBuffor, clients->name[index]);
            strcpy(tempBuffor, concat(tempBuffor, ": "));
            strcpy(tempBuffor, concat(tempBuffor, clients->message[index].message));

            int all = strcmp(clients->message[index].destName, "all");
            if(all == 0) {
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (clients->client_id[j] != -1 && j != index) {
                        sendback(clients->client_id[j], tempBuffor);
                    }
                }
            } else {
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (clients->client_id[j] != -1 && j != index) {
                        int dest = strcmp(clients->message[index].destName, clients->name[j]);
                        if(dest == 0) {
                            sendback(clients->client_id[j], tempBuffor);
                        }
                    }
                }
            }
        }
    }

    strcpy(clients->name[index],"\0");
    clients->client_id[index] = -1;
    close(clients->client_id[index]);
    sendOnlinelist(clients, index);

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


    for (int i = 0; i < MAX_CLIENTS; i++) {
        clientInfo.client_id[i] = -1;
        strcpy(clientInfo.name[i], "\0");
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

#ifndef INCLUDES_H
#define INCLUDES_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> //socket
#include <arpa/inet.h>  //inet_addr
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include "src/list.h"

#define ATTEMPTS 5

extern list_t* globalPeersList;
extern list_t* globalBlocksList;
extern list_t* globalFilesList;
extern list_t* threadList;

extern pthread_mutex_t peersAccess;
extern pthread_mutex_t blocksAccess;
extern pthread_mutex_t filesAccess;
extern pthread_mutex_t threadsAccess;
extern pthread_mutex_t clientsAccess;

extern sem_t semClean;
extern sem_t semFinish;
extern sem_t semCleanFinish;

extern unsigned int numberOfClients;

//Client data struct used for connection
typedef struct client
{
    int desc;
    struct in_addr ipAddr;
    pthread_t* thread;

} clientT;


//peer info
typedef struct peer
{
    char ip[20];
    int listeningSocket;

} peerT;


//File block struct
typedef struct fileBlock
{
    char fileName[100];
    unsigned int index;
    list_t* peers;

} fileBlockT;


//File struct
typedef struct file
{
    char fileName[100];
    unsigned int size;
    unsigned short isComplete;
    unsigned int numberOfBlocks;
    list_t* blocks; 
    char checksum[65];

} fileT;


#endif //INCLUDES_H
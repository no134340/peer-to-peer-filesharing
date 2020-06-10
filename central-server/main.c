/*
    ********************************************************************
    Odsek:          Elektrotehnika i racunarstvo
    Departman:      Racunarstvo i automatika
    Katedra:        Racunarska tehnika i racunarske komunikacije (RT-RK)
    Predmet:        Osnovi Racunarskih Mreza 1
    Godina studija: Treca (III)
    Skolska godina: 2019/2020
    Semestar:       Zimski (V)

    Ime fajla:      client.c
    Opis:           TCP/IP klijent

    Platforma:      Raspberry Pi 2 - Model B
    OS:             Raspbian
    ********************************************************************
*/

#include <fcntl.h>
#include <unistd.h>    
#include <sys/poll.h> //for file descriptor polling
#include "includes.h"
#include "src/jsonFunctions.h"
#include "src/list_functions.h"

#define DEFAULT_BUFLEN 1200
#define DEFAULT_PORT   27025
#define MAX_CONNECTIONS 20 // max **pending** connections at once

#define START 0
#define LS 1
#define REQUEST 2
#define RECEIVE_SUCCESS 3

list_t* globalPeersList;
list_t* globalBlocksList;
list_t* globalFilesList;
list_t* threadList;

pthread_mutex_t peersAccess;
pthread_mutex_t blocksAccess;
pthread_mutex_t filesAccess;
pthread_mutex_t threadsAccess;
pthread_mutex_t clientsAccess;

sem_t semClean;
sem_t semFinish;
sem_t semCleanFinish;

unsigned int numberOfClients;
int end_sig;

char getch(void);


int AddPeerToBlock(char* message, peerT* peer)
{
    fileBlockT* block = GetBlockInfo(message);
    if(block==NULL)
    {
        printf("Error finding block to add the peer.\n");
        fflush(stdout);
        return -1;
    }
    
    int listeningSocket = GetSocket(message);
    peerT* fPeer=NULL;
    pthread_mutex_lock(&peersAccess);
    
    int i;
    for(i=0; i < globalPeersList->len; i++)
    {
        list_node_t* pNode = list_at(globalPeersList,i);
        fPeer = (peerT*)pNode->val;
        if((strcmp(fPeer->ip,peer->ip)==0) && (fPeer->listeningSocket==listeningSocket))
        {
            break;
        }
    }
    pthread_mutex_unlock(&peersAccess);
    if(fPeer!=NULL)
    {
        list_node_t *peerNode = list_node_new(fPeer);
        list_rpush(block->peers, peerNode);
    }

    

    return 0;
}


int ProcessBlockRequest(char *message, clientT *data, peerT* peer)
{
    int readSize, command;
    int client_sock = data->desc;
    fileBlockT* requested_block;
    requested_block = GetBlockInfo(message);

    if(requested_block!=NULL)
    {
        char *client_message = GetPeerInfo(requested_block);

        if(send(client_sock, client_message, strlen(client_message),0)<0)
        {
            puts("Send peer info failed.");
            free(client_message);
            return -1;
        }
        free(client_message);
    }
    else
    {
        char *client_message = BlockNotFoundMessage();
        if(send(client_sock, message, strlen(message),0)<0)
        {
            puts("Send NOT_FOUND failed.");
            free(client_message);
            return -1;
        }
        free(client_message);
    }
    
    return 0;
}


int RemovePeer(peerT* peer)
{
    RemoveFromGlobal(peer);
    RemoveFromBlocks(peer);

    return 0;
}


int SendFilesList(clientT* peer)
{
    int client_sock = peer->desc;
    char *client_message = GenerateFilesListMessage();
    if(send(client_sock, client_message, strlen(client_message),0)<0)
    {
        puts("Send failed.");
        free(client_message);
        return -1;
    }

    free(client_message);

    return 0;
}


int AddBlocks(char *message, peerT* peer)
{
    if(GetBlocks(message, peer)!=0)
    {
        puts("Getting blocks error.\n");
        return -1;
    }

    return 0;
}


void* Client(void* args)
{
    pthread_mutex_lock(&clientsAccess);
    numberOfClients++;
    pthread_mutex_unlock(&clientsAccess);

    char tempBuffer[DEFAULT_BUFLEN];

    clientT* data = (clientT*) args;
    struct pollfd fds;
    fds.fd = data->desc;
    fds.events = POLLIN;

    //Receive client data
    char* message = (char*)malloc(DEFAULT_BUFLEN*sizeof(char));
    int readSize = 0;
    int noError = 1;

    //Create peer struct
    peerT* peer = (peerT*)malloc(sizeof(peerT));
    strcpy(peer->ip, inet_ntoa(data->ipAddr));

    int ret, counter = 0;
    while(noError)
    {
        if((ret=poll(&fds, 1, 5000))==0)
        {
            if(end_sig) break;
            continue;
        }

        readSize = recv(data->desc, tempBuffer, DEFAULT_BUFLEN, 0);
        if(readSize<=0) break;

        //process the message
        if(readSize==DEFAULT_BUFLEN && tempBuffer[DEFAULT_BUFLEN-1]!='\0')
        {
            memcpy(message+counter*DEFAULT_BUFLEN, tempBuffer, DEFAULT_BUFLEN);
            counter++;
            message = (char*)realloc(message, DEFAULT_BUFLEN*(counter+1));
            if(message==NULL)
            {
                printf("Out of memory!!!\n");   
                noError = 0;
                free(message);
                char* outOfMem = GenerateOOM();
                if(send(data->desc, outOfMem, strlen(outOfMem),0)<0)
                {
                    puts("Send OOM error failed.");
                }
                free(outOfMem);
                break;
            }
        }
        else
        {
            tempBuffer[readSize] = '\0';
            memcpy(message+counter*DEFAULT_BUFLEN, tempBuffer, readSize);

            int command = GetCommand(message);
            switch (command)
            {
                case START:
                    //add peer to global peer list
                    if(AddPeer(message,peer)!=0)
                    {
                        noError = 0;
                        break;
                    }
                    //add peer blocks to the global blocks list
                    if(AddBlocks(message, peer)==-1)
                    {
                        noError = 0;
                        break;
                    }
                    //add peer files to the global files list
                    if(GetFiles(message)!=0)
                        noError = 0;
                    break;
                case LS:
                    if(SendFilesList(data)==-1)
                        noError = 0;
                    break;
                case REQUEST:
                    if(ProcessBlockRequest(message, data, peer)==-1)
                        noError = 0;
                    break;
                case RECEIVE_SUCCESS:
                    if(AddPeerToBlock(message, peer)==-1)
                        noError = 0;
                    strcpy(message, "{\"command\":\"thank you\"}\0");
                    send(data->desc, message, strlen(message),0);
                    break;
                default:
                    puts("Invalid command.\n");
                    noError = 0;
                    break;
            }

            counter = 0;
            free(message);
            message = (char*) malloc(DEFAULT_BUFLEN*sizeof(char));
        }   
    }

    if(readSize == 0 && noError)
    {

        puts("Client disconnected");
        fflush(stdout);
    }
    else if(readSize== -1)
    {
        perror("Recv failed");
        fflush(stdout);
    }
    else if (noError==0 || end_sig)
    {
        puts("Server-side error or quit");
        fflush(stdout);  
    }

    /*
        Remove the client from all peers lists.
        Update block and file availability.
    */
    free(message);
    RemovePeer(peer);
    free(peer);
    CheckBlocksList();

    /*
        Close socket and free allocated memory.
    */
    close(data->desc);
    pthread_mutex_lock(&threadsAccess);
    list_rpush(threadList, list_node_new(data));
    pthread_mutex_unlock(&threadsAccess);
    sem_post(&semClean);
    return 0;
}


void* WaitForExit(void* args)
{
    char c;

    while (1)
    {
        if (sem_trywait(&semFinish) == 0)
        {
            break;
        }
        /*
            Get a character from the terminal. 
            The program is ended if the user inputs 'q' or 'Q'. 
         */
		c = getch();

        if (c == 'q' || c == 'Q')
        {
            /* 
                Signal all of the threads to end the program.
            */
            end_sig = 1;
            sem_post(&semFinish);
            sem_post(&semFinish);
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    end_sig = 0;
    numberOfClients = 0;
    srand(time(NULL));
    globalPeersList = list_new();
    globalBlocksList = list_new();
    globalFilesList = list_new();
    threadList = list_new();

    globalPeersList->match = MatchPeer;
    globalBlocksList->match = MatchBlock;
    globalFilesList->match = MatchFile;

    pthread_mutex_init(&peersAccess, NULL);
    pthread_mutex_init(&filesAccess, NULL);
    pthread_mutex_init(&blocksAccess, NULL);
    pthread_mutex_init(&threadsAccess, NULL);
    pthread_mutex_init(&clientsAccess, NULL);



    //data needed for connection
    struct sockaddr_in server, clientSocket;
    int socketDesc, clientDesc;
    int sockSize = sizeof(struct sockaddr_in);
    struct pollfd fds;


    //Create socket
    socketDesc = socket(AF_INET , SOCK_STREAM , 0);
    if (socketDesc == -1)
    {
        end_sig = 1;
        sem_post(&semFinish);
        printf("Could not create socket");
        goto exit_label;
    }
    puts("Socket created");


    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(DEFAULT_PORT);

    //Bind
	if(bind(socketDesc,(struct sockaddr*)&server , sizeof(server)) < 0)
	{
        end_sig = 1;
        sem_post(&semFinish);
		perror("Bind failed. Error");
		goto exit_label;
	}
	puts("Bind done");

    //cleanup thread
    pthread_t hCleaner;
    pthread_create(&hCleaner, NULL, CleanUp, 0);
    sem_init(&semClean, 0, 0);
    sem_init(&semCleanFinish, 0, 0);


    //wait for exit signal thread
    sem_init(&semFinish, 0, 0);
    pthread_t hFinish;
    pthread_create(&hFinish, NULL, WaitForExit, 0);

    //Listen
	listen(socketDesc , MAX_CONNECTIONS);
    puts("Waiting for incoming connections...");
    fds.fd = socketDesc;
    fds.events = POLLIN;
    /*  
        Poll the socket file descriptor to see if any cilents are incoming.
        If and only if the socket is ready to accept connections, accept it.
        (This was done so the main server thread isn't blocked on accept).
    */
    int ret;
    while(1)
	{
        if((ret=poll(&fds, 1, 5000))==0)
        {
            if(end_sig) break;
            continue;
        }
        clientDesc = accept(socketDesc, (struct sockaddr*) &clientSocket, (socklen_t*)&sockSize);
        puts("Connection accepted");
        //Get client data
		clientT* clientData = (clientT*) malloc(sizeof(clientT));

        struct sockaddr_in* clientSock = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
        memcpy(clientSock, &clientSocket,sizeof(struct sockaddr_in));

        clientData->ipAddr = clientSock->sin_addr; //get client IP
        clientData->desc = clientDesc;

        //Create thread to process client request
        pthread_t* hClient = (pthread_t*) malloc(sizeof(pthread_t));
        clientData->thread = hClient;
        
		pthread_create(hClient, NULL, Client, (void*)clientData);
	}



    pthread_join(hFinish, NULL);
    sem_post(&semCleanFinish);
    pthread_join(hCleaner, NULL);

exit_label:

    sem_destroy(&semClean);
    sem_destroy(&semFinish);
    sem_destroy(&semCleanFinish);
    
    pthread_mutex_destroy(&peersAccess);
    pthread_mutex_destroy(&filesAccess);
    pthread_mutex_destroy(&blocksAccess);
    pthread_mutex_destroy(&threadsAccess);
    pthread_mutex_destroy(&clientsAccess);

    list_destroy(threadList);
    list_destroy(globalFilesList);
    list_destroy(globalBlocksList);
    list_destroy(globalPeersList);

    if(socketDesc!=-1) 
        close(socketDesc);

	return 0;
}


//
// Created by dzuda11 on 7.12.19..
//

#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include "server_thread.h"
#include "TrashQueue.h"
#include "includes.h"
#include "./jsonFunctions.h"

#define DEFAULT_BUFLEN 512
extern const int listeningPort;
extern char *globalPathToDownloadsDir;

static pthread_t garbageCollector;

static sem_t garbageSem;

static void* ServerSubThread(void* pParam);
static void* GarbageCollectorThread(void* pParam);

void* ServerThread(void* pParam)
{
	// Start garbageCollector
	initTrashQueue();
	sem_init(&garbageSem, 0, 0);
	pthread_create(&garbageCollector, NULL, GarbageCollectorThread, NULL);
	pthread_detach(garbageCollector);
	//Create socket
	int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
		exit(21);
	}

	puts("[SERVER THREAD] Socket created");
	fflush(stdout);

	//Prepare the sockaddr_in structure
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(listeningPort);

	//Bind
	if(bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		//print the error message
		perror("[SERVER THREAD] Bind failed. Error");
		exit(22);
	}
	puts("[SERVER THREAD] Bind done");

	//Listen
	listen(socket_desc , SOMAXCONN);

	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	int c = sizeof(struct sockaddr_in);

	struct sockaddr_in* client = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
	int* client_sock = (int*) malloc(sizeof(int));
	while ((*client_sock = accept(socket_desc, (struct sockaddr *)client, (socklen_t*)&c)) >=0)
	{
		if (*client_sock < 0)
		{
			perror("Accept failed. Error");
		}
		// Create and fill SubThreadArg structure
		SubThreadArg* arg = (SubThreadArg*) malloc(sizeof(SubThreadArg));
		arg -> client = client;
		arg -> client_sock = client_sock;
		arg -> threadPointer = (pthread_t*) malloc(sizeof(pthread_t));
		arg -> selfPointer = arg;
		fflush(stdout);
		pthread_create(arg -> threadPointer, NULL, ServerSubThread, (void*)arg);

		//Create new client and client_sock
		client = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
		client_sock = (int*) malloc(sizeof(int));
	}

	return 0;
}

static void* GarbageCollectorThread(void* pParam)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
	while (1)
	{
		sem_wait(&garbageSem);
		struct SubThreadArg* arg = popTrash();
		pthread_join(*arg -> threadPointer, NULL);
		free(arg -> client_sock);
		free(arg->client);
		free(arg -> threadPointer);
		free(arg->selfPointer);
	}
#pragma clang diagnostic pop
}

static void* ServerSubThread(void* pParam)
{
	SubThreadArg* arg = (SubThreadArg*) pParam;
	struct sockaddr_in client = *arg -> client;
	int client_sock = *arg -> client_sock;


	char* client_message = (char*) malloc(DEFAULT_BUFLEN);
	char* clientBuffer = (char*) malloc(DEFAULT_BUFLEN);
	if (client_message == NULL || clientBuffer == NULL)
	{
		puts("[SERVER_SUB_THREAD] Failed to allocate memory");
		pushTrash(arg);
		sem_post(&garbageSem);
		return 1;
	}
	int read_size;
	int blockIndex;
	while(1)
	{
		int counter = 0;
		read_size = recv(client_sock , clientBuffer , DEFAULT_BUFLEN , 0);
		if (read_size <= 0)
			break;
		while (read_size == DEFAULT_BUFLEN && clientBuffer[DEFAULT_BUFLEN - 1] != 0)
		{
			memcpy(client_message + counter*DEFAULT_BUFLEN, clientBuffer, DEFAULT_BUFLEN);
			client_message = (char*) realloc(client_message, (++counter + 1) * DEFAULT_BUFLEN);
			if (client_message == NULL)
			{
				puts("[SERVER_SUB_THREAD] Failed to allocate memory");
				pushTrash(arg);
				sem_post(&garbageSem);
				return 1;
			}
			read_size = recv(client_sock , clientBuffer , DEFAULT_BUFLEN , 0);
		}
		clientBuffer[read_size] = 0;
		memcpy(client_message + counter * DEFAULT_BUFLEN, clientBuffer, strlen(clientBuffer));
		printf("Bytes received: %lu\n", strlen(client_message));
		printf("%s\n", client_message);
		
		
		//parse the client message to get filename and block index info
		char *fileName;

		int blockSize;
		getBlockInfo(&blockIndex, &blockSize, &fileName, client_message);
		char *filePath = (char *)malloc(strlen(globalPathToDownloadsDir)+1+strlen(fileName));
		strcpy(filePath, globalPathToDownloadsDir);
		strcat(filePath,"/");
		strcat(filePath, fileName);

		//open that file, read the indexed block and send it to the peer
		FILE *fp = fopen(filePath,"r");
    	if (fp == NULL)
		{
			printf("Can't open the file %s!\n", filePath);
			fflush(stdout);
			free(filePath);
			free(fileName);
			free(client_message);
			pushTrash(arg);
			sem_post(&garbageSem);
			return 4;
    	}

		char *blockPart = (char*) malloc(MAX_MESSAGE_LENGTH);
		long offset = blockIndex*blockSize;
		fseek(fp, offset, SEEK_SET);
		counter = blockSize;
		while(counter>0)
		{
			int readLen = counter > MAX_MESSAGE_LENGTH ? MAX_MESSAGE_LENGTH : counter;
			size_t len = fread(blockPart, sizeof(char), readLen, fp);
		
			if(send(client_sock, blockPart, len, 0) < 0)
			{
				printf("Send of file %s, block index %d failed!\n", filePath, blockIndex);
				fflush(stdout);
				free(blockPart);
				free(filePath);
				free(fileName);
				free(client_message);
				pushTrash(arg);
				sem_post(&garbageSem);
				return 5;
			}

			counter -= readLen;
		}
		free(blockPart);
		free(filePath);
		free(fileName);
		free(client_message);
	}


	pushTrash(arg);
	sem_post(&garbageSem);
	if(read_size == 0)
	{
		puts("[SERVER_SUB_THREAD] Client disconnected");
		fflush(stdout);
		return 0;
	}
	else if(read_size == -1)
	{
		printf("[SERVER_SUB_THREAD] Index %d", blockIndex);
		fflush(stdout);
		perror("Receive failed");
		return 2;
	}
	return 3;

}

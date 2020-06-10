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


#include<stdio.h>      //printf
//#include "include/parson.h"
#include "src/jsonFunctions.h"
#include <stdlib.h>
#include<string.h>     //strlen
#include<sys/socket.h> //socket
#include<arpa/inet.h>  //inet_addr
#include <fcntl.h>     //for open
#include <unistd.h>    //for close
#include <ifaddrs.h>
#include <pthread.h>

#include "src/server_thread.h"
#include "src/client_thread.h"

pthread_t serverThread;
pthread_t clientThread;

int listeningPort = 27000;
char *globalPathToDownloadsDir;
in_addr_t globalServerIP;

static void InitGlobalVar(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	InitGlobalVar(argc, argv);

	pthread_create(&clientThread, NULL, ClientThread, 0);
	pthread_create(&serverThread, NULL, ServerThread, 0);
	pthread_join(clientThread, NULL);
	pthread_join(serverThread, NULL);
	return 0;
}

static void InitGlobalVar(int argc, char *argv[])
{
	globalPathToDownloadsDir = malloc(512);
	if (globalPathToDownloadsDir == NULL)
	{
		printf("Could not allocate enough memory for \"globalPathToDownloadsDir\"");
		exit(3);
	}
	if(argc > 1)
	{
		globalServerIP = inet_addr(argv[1]);
		if (globalServerIP == INADDR_NONE)
		{
			printf("Bad arguments. Could not parse server IP\n");
			exit(1);
		}
		printf("Server IP address: %s\n", argv[1]);
	}
	else
	{
		globalServerIP = inet_addr("127.0.0.1");
		printf("Using default server IP address: \"127.0.0.1\"\n");
	}

	if (argc > 2)
	{
		globalPathToDownloadsDir = strcpy(globalPathToDownloadsDir, argv[2]);
		printf("Downloads directory: %s\n", globalPathToDownloadsDir);
	}
	else
	{
		globalPathToDownloadsDir = strcpy(globalPathToDownloadsDir, "../downloads");
		printf("Using default path to download directory: %s\n", globalPathToDownloadsDir);
	}

	if (argc > 2)
	{
		listeningPort = atoi(argv[3]);
		printf("Listening port: %d\n", listeningPort);
	}
	else
	{
		printf("Default listening port: %d\n", listeningPort);
	}
}


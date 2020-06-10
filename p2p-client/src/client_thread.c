//
// Created by dzuda11 on 7.12.19..
//

#include "client_thread.h"
#include "jsonFunctions.h"
#include "../include/list.h"
#include "ClientTrheadSrc/ListLocalFiles.h"
#include "ClientTrheadSrc/DownloadFile.h"
#include "ClientTrheadSrc/CreateMetaFile.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define DEFAULT_PORT   27025

extern char* globalPathToDownloadsDir;
extern in_addr_t globalServerIP;

static void HandleListLocalFiles(char* command);
static void HandleListNetFiles(char* command, int sock);
static void HandleCreateMetaFile(char *command, int sock);
static void HandleDownloadFile(char *command, int sock);
static void HandleHelp(char* command);

void* ClientThread(void* pParam)
{
	int sock;
	struct sockaddr_in server;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		printf("[CLIENT_THREAD] Could not create socket");
		return 1;
	}
	puts("[CLIENT_THREAD] Socket created");

	server.sin_addr.s_addr = globalServerIP;
	server.sin_family = AF_INET;
	server.sin_port = htons(DEFAULT_PORT);

	if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		perror("[CLIENT_THREAD] Connect failed. Error");
		return 1;
	}
	puts("Connected\n");

	char* clientHelloMessage = GenerateC2SHelloMessage();
	if (send(sock, clientHelloMessage, strlen(clientHelloMessage), 0) < 0)
	{
		puts("[CLIENT_THREAD] Send failed");
	}
	json_free_serialized_string(clientHelloMessage);

	while(1)
	{
		char userCommand[512];
		printf(">> ");
		gets(userCommand);

		if(strcmp(userCommand, "exit") == 0)
		{
			break; //  TODO Close server thread
			exit(0);
		}
		if (strncmp(userCommand, "clear", strlen("clear")) == 0)
		{
			printf("\x1b[H\x1b[J");
		}
		else if (strncmp(userCommand, "create", strlen("create")) == 0)
		{
			HandleCreateMetaFile(userCommand, sock);
		}
		else if (strncmp(userCommand, "download", strlen("download")) == 0)
		{
			HandleDownloadFile(userCommand, sock);
		}
		else if (strcmp(userCommand, "help") == 0)
		{
			HandleHelp(userCommand);
		}
		else if (strcmp(userCommand, "ll") == 0)
		{
			HandleListLocalFiles(userCommand);
		}
		else if (strcmp(userCommand, "ls") == 0)
		{
			HandleListNetFiles(userCommand, sock);
		}
		else
		{
			puts("Unknown command. Try 'help'");
		}
	}
	close(sock);

}

/*int LocalFilesListCompare(void* x, void* y)
{
	if(strcmp(x, y) == 0)
		return 1;
	x += 256; y += 256;		// Because why not?
	if(strcmp(x, y) == 0)
		return 1;
	return 0;
}*/

void HandleListLocalFiles(char *command)
{
	list_t *fileList = list_new();
	if (fileList == NULL)
	{
		puts("[ListLocalFiles] Failed to create files list!");
		return;
	}

	if(FillListWithFiles(fileList, globalPathToDownloadsDir) == -1)
		return;

	printFileList(fileList);

	// Free list and memory
	list_node_t *node;
	while ((node = list_rpop(fileList)))
	{
		free(node->val);
	}
	list_destroy(fileList);
}

void HandleListNetFiles(char *command, int sock)
{
	char *clientCommandMessage = "{\"command\" : \"fileList\"}\n";
	if (send(sock, clientCommandMessage, strlen(clientCommandMessage), 0) < 0)
	{
		puts("[CLIENT_THREAD][ListNetFiles] Sending command failed");
	}

	char *response = malloc(MAX_MESSAGE_LENGTH);
	int readSize = recv(sock, response, MAX_MESSAGE_LENGTH, 0);
	if (readSize == 0)
	{
		puts("[CLIENT_THREAD][ListNetFiles] Receive empty string");
		free(response);
		return;
	}

	puts("FILES AVAILABLE ON NETWORK");
	JSON_Value* root_value = json_parse_string(response);
	JSON_Array *fileArray = json_object_get_array(json_value_get_object(root_value), "files");
	for(int i = 0; i < json_array_get_count(fileArray); i++)
	{
		JSON_Object* file = json_value_get_object(json_array_get_value(fileArray, i));
		printf("\t%d\t%s\t\t\t%d kB\n", i + 1,
				json_object_get_string(file, "fileName"),
			   (unsigned int)json_object_get_number(file, "size")/1024);
	}
	free(response);
	json_value_free(root_value);
}

void HandleCreateMetaFile(char *command, int sock)
{
	char* fileName = strchr(command, ' ') + 1;
	if (fileName == (void*)0x1)
	{
		printf("Error: Bad arguments\n");
		return;
	}
	char* blockSizeString = strrchr(fileName, ' ') + 1;
	int numOfBlocks;
	if(blockSizeString == (void*)0x1)
	{
		numOfBlocks = 0; // default value
	} else
	{
		*(blockSizeString - 1) = '\0';
		numOfBlocks = atoi(blockSizeString);
	}

	char* pathToFile = (char*)malloc(strlen(globalPathToDownloadsDir) + 1 + strlen(fileName));
	strcpy(pathToFile, globalPathToDownloadsDir);
	strcat(pathToFile, "/");
	strcat(pathToFile, fileName);
	if (access(pathToFile, F_OK) != 0)
	{
		printf("File doesn't exist!\n");
		free(pathToFile);
		return;
	}
	char* pathToMeta = (char*)malloc(strlen(pathToFile) + strlen(META_FILE_EXTENSION));
	strcpy(pathToMeta, pathToFile);
	strcat(pathToMeta, META_FILE_EXTENSION);
	if(access(pathToMeta, F_OK) == 0)
	{
		printf("Meta file already exist\n");
		free(pathToFile);
		free(pathToMeta);
		return;
	}
	free(pathToMeta);

	// Check does file exist in peer-to-peer network
	char *clientCommandMessage = "{\"command\" : \"fileList\"}\n";
	if (send(sock, clientCommandMessage, strlen(clientCommandMessage), 0) < 0)
	{
		puts("[CLIENT_THREAD][CreateMetaFile] Sending command failed\n");
		free(pathToFile);
		return;
	}

	char *response = malloc(MAX_MESSAGE_LENGTH);
	int readSize = recv(sock, response, MAX_MESSAGE_LENGTH, 0);
	if (readSize == 0)
	{
		puts("[CLIENT_THREAD][CreateMetaFile] Receive empty string\n");
		free(pathToFile);
		free(response);
		return;
	}

	JSON_Value* root_value = json_parse_string(response);
	JSON_Array *fileArray = json_object_get_array(json_value_get_object(root_value), "files");
	for(int i = 0; i < json_array_get_count(fileArray); i++)
	{
		JSON_Object* file = json_value_get_object(json_array_get_value(fileArray, i));
		if (strcmp(json_object_get_string(file, "fileName"), fileName))
		{
			continue;
		}
		if((int)json_object_get_number(file, "numOfBlocks") != numOfBlocks)
		{
			numOfBlocks = (int)json_object_get_number(file, "numOfBlocks");
			printf("Meta file is available on network. Creating meta file with %d blocks\n", numOfBlocks);
		}
		break;
	}
	free(response);
	json_value_free(root_value);

	if(numOfBlocks)
	{
		CreateMetaFile(pathToFile, numOfBlocks);
	} else
	{
		printf("File is not available on network, you should define number of blocks\n");
	}
	free(pathToFile);
}

void HandleDownloadFile(char *command, int sock)
{
	char* fileName = strrchr(command, ' ') + 1;
	printf("Start downloading '%s'\n", fileName);

	//First check dose file exist and num of blocks - Using same request like in ListNetFiles
	char *clientCommandMessage = "{\"command\" : \"fileList\"}\n";
	if (send(sock, clientCommandMessage, strlen(clientCommandMessage), 0) < 0)
	{
		puts("[CLIENT_THREAD][DownloadFIle] Sending command failed\n");
		return;
	}

	char *response = malloc(MAX_MESSAGE_LENGTH);
	int readSize = recv(sock, response, MAX_MESSAGE_LENGTH, 0);
	if (readSize == 0)
	{
		puts("[CLIENT_THREAD][DownloadFIle] Receive empty string\n");
		free(response);
		return;
	}

	JSON_Value* root_value = json_parse_string(response);
	JSON_Array *fileArray = json_object_get_array(json_value_get_object(root_value), "files");
	char flag = 0;
	for(int i = 0; i < json_array_get_count(fileArray); i++)
	{
		JSON_Object* file = json_value_get_object(json_array_get_value(fileArray, i));
		if (strcmp(json_object_get_string(file, "fileName"), fileName))
		{
			continue;
		}
		DownloadFile(fileName, globalPathToDownloadsDir,
					 (unsigned int) json_object_get_number(file, "size"),
					 (unsigned int) json_object_get_number(file, "numOfBlocks"),
					 json_object_get_string(file, "checkSum"),
					 globalServerIP,
					 DEFAULT_PORT);
		flag = 1;
		break;
	}
	if(!flag)
	{
		printf("File '%s' is not available on this network\n", fileName);
	}
	free(response);
	json_value_free(root_value);
}

void HandleHelp(char *command)
{
	puts("Welcome to P2P files share client");
	puts("Available commands are:");
	puts("");
	puts("\t'create fileName' - This command create meta file for new file in network");
	puts("\t'download fileName' - This command start downloading file from p2p network");
	puts("\t'ls' - list all completed files from network");
	puts("\t'll' - list files on local and info about them");
	puts("\t'exit' - exit application");
	puts("");
}

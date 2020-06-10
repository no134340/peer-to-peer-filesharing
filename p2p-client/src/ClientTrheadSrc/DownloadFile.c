//
// Created by dzuda11 on 3.1.20..
//
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "DownloadFile.h"
#include "../../include/parson.h"
#include "../sha256sum.h"

extern int listeningPort;

struct BlockThreadArg
{
	char *fileName;
	unsigned int index;
	unsigned int size;
	int sock;
	FILE* fp;
	char* metaFileName;
	pthread_mutex_t* fileMutex;
	pthread_mutex_t* metaFileMutex;
	pthread_mutex_t* socketMutex;
};

static void* DownloadBlock(void* pParam)
{
	struct BlockThreadArg *arg = (struct BlockThreadArg*) pParam;

	char *clientCommandMessage = (char*)malloc(256);
	sprintf(clientCommandMessage, "{\"command\":\"download\",\"parentFileName\":\"%s\",\"index\":%d,\"size\":%d}",
			arg->fileName, arg->index, arg->size);

	pthread_mutex_lock(arg->socketMutex);
	if (send(arg->sock, clientCommandMessage, strlen(clientCommandMessage), 0) < 0)
	{
		printf("[CLIENT_THREAD][DownloadBlock][%s][%d] Send fail\n>>", arg->fileName, arg->index);
	}

	char *response = malloc(512);
	int readSize = recv(arg->sock, response, 512, 0);
	if (readSize == 0)
	{
		printf("[CLIENT_THREAD][DownloadBlock][%s][%d] Receive empty string\n>>", arg->fileName, arg->index);
		free(response);
		return 1;
	}
	pthread_mutex_unlock(arg->socketMutex);


	JSON_Value* rootValue = json_parse_string(response);
	JSON_Object* rootObject = json_value_get_object(rootValue);

	int sock;
	struct sockaddr_in server;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		printf("[CLIENT_THREAD][DownloadBlock][%s][%d] Could not create socket\n>>", arg->fileName, arg->index);
		free(response);
		return 1;
	}

	in_addr_t peerIP = inet_addr(json_object_get_string(rootObject, "IP"));
	if(peerIP == INADDR_NONE)
	{
		printf("[CLIENT_THREAD][DownloadBlock][%s][%d] Invalid peer IP\n>>", arg->fileName, arg->index);
		free(response);
		return 1;
	}
	server.sin_addr.s_addr = peerIP;
	server.sin_family = AF_INET;
	server.sin_port = htons((int)json_object_get_number(rootObject, "socket"));
	free(response);

	if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		printf("[CLIENT_THREAD][DownloadBlock][%s][%d]\n", arg->fileName, arg->index);
		perror(" Connect failed. Error");
		return 1;
	}

	clientCommandMessage = (char*)malloc(256);
	sprintf(clientCommandMessage, "{\"command\":\"download\",\"parentFileName\":\"%s\",\"index\":%d, \"size\":%d}",
			arg->fileName, arg->index, arg->size); // TODO check this

	if (send(sock, clientCommandMessage, strlen(clientCommandMessage), 0) < 0)
	{
		printf("[CLIENT_THREAD][DownloadBlock][%s][%d] Sending command failed\n>>", arg->fileName, arg->index);
	}

	response = (char*)malloc(MAX_MESSAGE_LENGTH);

	unsigned int subBlocksNum = arg->size % MAX_MESSAGE_LENGTH ? arg->size / MAX_MESSAGE_LENGTH + 1 : arg->size / MAX_MESSAGE_LENGTH;
	for (int i = 0; i < subBlocksNum; ++i)
	{
		readSize = recv(sock, response, MAX_MESSAGE_LENGTH, 0);
		if(readSize == 0)
		{
			printf("[CLIENT_THREAD][DownloadBlock][%s][%d] Receive empty string from peer\n>>", arg->fileName, arg->index);
			free(response);
			return 1;
		}

		// TODO write response to file on right position
		pthread_mutex_lock(arg->fileMutex);
		if(fseek(arg->fp, arg->index * arg->size + i * MAX_MESSAGE_LENGTH, SEEK_SET) != 0)
		{
			printf("[CLIENT_THREAD][DownloadBlock][%s][%d] Setting file position failed!!\n>>", arg->fileName, arg->index);
			pthread_mutex_unlock(arg->fileMutex);
			return 1;
		}
		fwrite(response, 1, readSize, arg->fp);
		pthread_mutex_unlock(arg->fileMutex);
	}
	close(sock);

	//TODO Write info about new block to meta file and notify central-server
	JSON_Value *block_value = json_value_init_object();
	JSON_Object *block_object = json_value_get_object(block_value);
	json_object_set_string(block_object, "parentFile", arg->fileName);
	json_object_set_number(block_object, "index", arg->index);
	json_object_set_number(block_object, "size", arg->size);

	pthread_mutex_lock(arg-> metaFileMutex);
	JSON_Value* root_value = json_parse_file(arg->metaFileName);
	JSON_Object *root_object = json_value_get_object(root_value);
	json_array_append_value(json_object_get_array(root_object, "blocks"), block_value);
	json_serialize_to_file_pretty(root_value, arg->metaFileName);
	json_value_free(root_value);
	pthread_mutex_unlock(arg-> metaFileMutex);


	sprintf(clientCommandMessage, "{\"command\":\"success\",\"parentFileName\":\"%s\",\"index\":%d,\"size\":%d, \"listeningPort\":%d}",
			arg->fileName, arg->index, arg->size, listeningPort);

	pthread_mutex_lock(arg->socketMutex);
	if (send(arg->sock, clientCommandMessage, strlen(clientCommandMessage), 0) < 0)
	{
		printf("[CLIENT_THREAD][DownloadBlock][%s][%d] Send fail\n>>", arg->fileName, arg->index);
	}

	readSize = recv(arg->sock, response, 512, 0);
	if (readSize == 0)
	{
		printf("[CLIENT_THREAD][DownloadBlock][%s][%d] Receive empty string\n>>", arg->fileName, arg->index);
		free(response);
		return 1;
	}
	pthread_mutex_unlock(arg->socketMutex);
	free(clientCommandMessage);
	free(response);
}

void DownloadFile(char *fileName, const char *downloadDir, unsigned int size, unsigned int numOfBlocks,
					const char *checkSum, in_addr_t serverIP, int port)
{
	// Creating socket for communication with server
	struct sockaddr_in server;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		printf("[CLIENT_THREAD][Download File] Could not create socket");
		return;
	}

	server.sin_addr.s_addr = serverIP;
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		perror("[CLIENT_THREAD][Download File] Connect failed. Error");
		return;
	}

	pthread_t** blockThreads = (pthread_t**)malloc(numOfBlocks * sizeof(pthread_t*));
	struct BlockThreadArg* arguments = (struct BlockThreadArg*)malloc(numOfBlocks * sizeof(struct BlockThreadArg));

	// Creating file
	char* partFileName = (char*)malloc(strlen(downloadDir) + 1 + strlen(fileName) + strlen(PART_FILE_EXTENSION));
	strcpy(partFileName, downloadDir);
	strcat(partFileName, "/");
	strcat(partFileName, fileName);
	strcat(partFileName, PART_FILE_EXTENSION);
	FILE* fp = fopen(partFileName, "w");
	fseek(fp, size - 1, SEEK_SET);
	fputc(EOF, fp);

	// Creating meta file
	char* metaFileName = (char*)malloc(strlen(partFileName) + strlen(META_FILE_EXTENSION));
	strcpy(metaFileName, partFileName);
	*(metaFileName + strlen(metaFileName) - strlen(PART_FILE_EXTENSION)) = '\0';
	strcat(metaFileName, META_FILE_EXTENSION);

	JSON_Value* root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);
	json_object_set_string(root_object, "fileName", fileName);
	json_object_set_number(root_object, "size", size);
	json_object_set_number(root_object, "numOfBlocks", numOfBlocks);
	json_object_set_string(root_object, "checkSum", checkSum);
	json_object_dotset_value(root_object, "blocks", json_value_init_array());

	json_serialize_to_file_pretty(root_value, metaFileName);
	json_value_free(root_value);


	pthread_mutex_t fileMutex;
	pthread_mutex_init(&fileMutex, NULL);

	pthread_mutex_t metaFileMutex;
	pthread_mutex_init(&metaFileMutex, NULL);

	pthread_mutex_t socketMutex;
	pthread_mutex_init(&socketMutex, NULL);

	int i;
	for (i = 0; i < numOfBlocks; i++)
	{
		arguments[i].fileName = fileName;
		arguments[i].index = i;
		arguments[i].size = size % numOfBlocks ? size / numOfBlocks + 1 : size / numOfBlocks;
		arguments[i].sock = sock;
		arguments[i].fp = fp;
		arguments[i].metaFileName = metaFileName;
		arguments[i].fileMutex = &fileMutex;
		arguments[i].metaFileMutex = &metaFileMutex;
		arguments[i].socketMutex = &socketMutex;

		blockThreads[i] = (pthread_t*)malloc(sizeof(pthread_t));
		pthread_create(blockThreads[i], NULL, DownloadBlock, &arguments[i]);
	}

	for(i = 0; i< numOfBlocks; i++)
	{
		pthread_join(*blockThreads[i], NULL);
		free(blockThreads[i]);
	}

	close(sock);
	free(blockThreads);
	free(arguments);
	free(metaFileName);

	fclose(fp);

	char* pathToFile = malloc(strlen(partFileName));
	strcpy(pathToFile, partFileName);
	*(strrchr(pathToFile, '.')) = '\0';

	rename(partFileName, pathToFile);

	char myFileCheckSum[65];
	sha256sum(pathToFile, myFileCheckSum);

	free(partFileName);
	free(pathToFile);

	if(strcmp(checkSum, myFileCheckSum) != 0)
	{
		printf("\rERROR!! Checksums are not identical! File - %s\n", fileName);

	} else
	{
		printf("\rDownload completed - file '%s' is ready!\n", fileName);
	}
}

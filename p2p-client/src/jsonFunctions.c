//
// Created by dzuda11 on 3.12.19..
//
#include "jsonFunctions.h"
#include <string.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <arpa/inet.h>


extern const int listeningPort;
extern char * globalPathToDownloadsDir;

static int GetFilesAndBlocksJsonFrom(JSON_Array *fileArray, JSON_Array *blocksArray, const char* dir)
{
	struct dirent *de;  // Pointer for directory entry
	DIR *dr = opendir(dir);

	if (dr == NULL)  // opendir returns NULL if couldn't open directory
	{
		printf("Could not open downloads directory");
		exit(2);
		return 1;
	}

	char *pathToFile = malloc(512);
	if (pathToFile == NULL)
	{
		printf("Could not allocate enough memory for \"pathToFile\"");
		exit(3);
	}
	while ((de = readdir(dr)) != NULL)
	{
		if(strlen(de->d_name) < strlen(META_FILE_EXTENSION))
			continue;
		if(strncmp(de->d_name + strlen(de->d_name) - strlen(META_FILE_EXTENSION), META_FILE_EXTENSION, strlen(META_FILE_EXTENSION)) != 0)
		{
			continue; // if dose not have rigrt extensin conturn
		}
		pathToFile = memcpy(pathToFile, dir, strlen(dir)+1);
		pathToFile = strcat(pathToFile, "/");
		pathToFile = strcat(pathToFile, de->d_name);

		JSON_Value *file_value = json_parse_file(pathToFile);
		if (file_value == NULL)
			continue;
		JSON_Object *file_object = json_value_get_object(file_value);
		JSON_Array *files_blocks = json_object_get_array(file_object, "blocks");
		const char * fileName = json_object_get_string(file_object, "fileName");
		for(int i = 0; i < json_array_get_count(files_blocks); i++)
		{
			JSON_Value* tmp = json_array_get_value(files_blocks, i);
			json_object_set_string(json_value_get_object(tmp), "fileName", fileName);

			char *mile = json_serialize_to_string_pretty(tmp);
			JSON_Value* root_value = json_parse_string(mile);
			json_array_append_value(blocksArray, root_value);
		}
		json_object_remove(file_object, "blocks");
		json_array_append_value(fileArray, file_value);
		//json_value_free(file_value); // WHY ???
	}
	free(pathToFile);
	closedir(dr);
}

char *GenerateC2SHelloMessage()
{
	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);
	char *serialized_string = NULL;

	json_object_set_string(root_object, "command", "clientFilesAndBlocks");

	json_object_set_number(root_object, "pearListeningSocket", listeningPort);

	json_object_dotset_value(root_object, "files", json_value_init_array());
	JSON_Array *fileArray = json_object_get_array(root_object, "files");

	json_object_dotset_value(root_object, "blocks", json_value_init_array());
	JSON_Array *blocksArray = json_object_get_array(root_object, "blocks");

	GetFilesAndBlocksJsonFrom(fileArray, blocksArray, globalPathToDownloadsDir); // static function see above

	serialized_string = json_serialize_to_string_pretty(root_value);

	json_value_free(root_value);

	return serialized_string;
}

int getBlockInfo(int* index, int* blockSize, char** fileName, char* jsonFile)
{
	
    JSON_Value *rootValue  = json_parse_string(jsonFile);
	size_t nameLen = strlen(json_object_get_string(json_value_get_object(rootValue), "parentFileName"));
	*fileName = (char *) malloc(nameLen+1);
    strcpy(*fileName,json_object_get_string(json_value_get_object(rootValue), "parentFileName"));
    *index = (int)json_object_get_number(json_value_get_object(rootValue),"index");
	*blockSize = json_object_get_number(json_value_get_object(rootValue),"size");

    json_value_free(rootValue);

	return 0;
}
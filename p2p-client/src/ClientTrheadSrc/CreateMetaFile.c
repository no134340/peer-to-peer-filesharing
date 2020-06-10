//
// Created by dzuda11 on 14.1.20..
//

#include "CreateMetaFile.h"
#include "../../include/parson.h"

void CreateMetaFile(char* pathToFile, unsigned int numOfBlocks)
{
	char checkSum[65];
	sha256sum(pathToFile, checkSum);

	FILE* fp = fopen(pathToFile, "r");
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	if (size == -1)
	{
		printf("Create file error. Can't calculate file size\n");
		fclose(fp);
		return;
	}

	JSON_Value *root_value = json_value_init_object();
	JSON_Object *root_object = json_value_get_object(root_value);

	json_object_set_string(root_object, "fileName", strrchr(pathToFile, '/') + 1);
	json_object_set_number(root_object, "size", size);
	json_object_set_number(root_object, "numOfBlocks", numOfBlocks);
	json_object_set_string(root_object, "checkSum", checkSum);
	json_object_dotset_value(root_object, "blocks", json_value_init_array());

	for(int i = 0; i < numOfBlocks; i++)
	{
		JSON_Value *block_value = json_value_init_object();
		JSON_Object *block_object = json_value_get_object(block_value);
		json_object_set_string(block_object, "parentFile", strrchr(pathToFile, '/') + 1);
		json_object_set_number(block_object, "index", i);
		json_object_set_number(block_object, "size", size % numOfBlocks ? size / numOfBlocks + 1 : size / numOfBlocks);

		json_array_append_value(json_object_get_array(root_object, "blocks"), block_value);
	}

	char* pathToMeta = (char*)malloc(strlen(pathToFile) + strlen(META_FILE_EXTENSION));
	strcpy(pathToMeta, pathToFile);
	strcat(pathToMeta, META_FILE_EXTENSION);

	json_serialize_to_file_pretty(root_value, pathToMeta);
	json_value_free(root_value);

	free(pathToMeta);
}

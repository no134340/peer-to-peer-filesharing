#include "jsonFunctions.h"


int GetCommand(char* jsonFile)
{
    JSON_Value *rootValue  = json_parse_string(jsonFile);
    char command[512];
    strcpy(command,json_object_get_string(json_value_get_object(rootValue), "command"));
    json_value_free(rootValue);
    
    if(strcmp(command,"clientFilesAndBlocks")==0)
        return 0;
    else if(strcmp(command,"fileList")==0)
        return 1;
    else if(strcmp(command, "download")==0)
        return 2;
    else if(strcmp(command, "success")==0)
        return 3;
    else
        return -1;
}



int AddPeer(char *jsonFile, peerT* peer)
{
    JSON_Value *rootValue  = json_parse_string(jsonFile);
    
    int listeningSocket = (int)json_object_get_number(json_value_get_object(rootValue), "pearListeningSocket");
    peer->listeningSocket = listeningSocket;
    list_node_t* peerNode = list_node_new(peer);
    pthread_mutex_lock(&peersAccess);
    list_rpush(globalPeersList, peerNode);
    pthread_mutex_unlock(&peersAccess);
    return 0;
}

int GetBlocks(char* jsonFile, peerT* peer)
{
    JSON_Value *rootValue  = json_parse_string(jsonFile);
    
    /*
        Get each block information from client json file.
        If block already exists, add the peer to the block's peer list;
    */
    JSON_Array *blocks = json_object_get_array(json_value_get_object(rootValue), "blocks");
    int numberOfBlocks = json_array_get_count(blocks);
    int i;
    pthread_mutex_lock(&blocksAccess);
    for (i = 0; i < numberOfBlocks; i++)
    {
        //make block node and push it to the list
        JSON_Value* tmp = json_array_get_value(blocks,i);
        fileBlockT *block = (fileBlockT*)malloc(sizeof(fileBlockT));
        strcpy(block->fileName, json_object_get_string(json_value_get_object(tmp), "parentFile"));
        block->index = (unsigned int)json_object_get_number(json_value_get_object(tmp),"index");

        list_node_t *findNode = NULL;
        
        if(globalBlocksList->head!=NULL) findNode=list_find(globalBlocksList, block);
        if(findNode!=NULL)// check if block information already exists in the global block list
        {
            fileBlockT* foundBlock = (fileBlockT*) findNode->val;
            list_node_t* peerNodeBlock = list_node_new(peer);
            list_rpush(foundBlock->peers, peerNodeBlock);
            free(block);
        }
        else
        {
            list_t* peers = list_new();
            block->peers = peers;
            list_node_t* peerNodeBlock = list_node_new(peer);
            list_rpush(block->peers, peerNodeBlock);
            list_node_t* blockNode = list_node_new(block);
            list_rpush(globalBlocksList, blockNode);
        }
    }
    pthread_mutex_unlock(&blocksAccess);

    json_value_free(rootValue);

    return 0;
}


int MakeBlocksList(fileT *file)
{
    int i;
    pthread_mutex_lock(&blocksAccess);
    for(i = 0; i < globalBlocksList->len; i++)
    {
        list_node_t *blockNode = list_at(globalBlocksList,i);
        if(blockNode!=NULL)
        {          
            fileBlockT *block = (fileBlockT *) blockNode->val;
            if(list_find(file->blocks, block)==NULL)
            {
                if (strcmp(file->fileName, block->fileName) == 0)
                {
                    list_node_t *fileBlockNode= list_node_new(block);
                    list_rpush(file->blocks, fileBlockNode);
                }
            }
        } 
        else        
        {
            pthread_mutex_unlock(&blocksAccess);
            return 1;
        }
    }
    pthread_mutex_unlock(&blocksAccess);
    return 0;
}


int GetFiles(char* jsonFile)
{
    JSON_Value *rootValue  = json_parse_string(jsonFile);
    JSON_Array *files = json_object_get_array(json_value_get_object(rootValue), "files");
    int numberOfFiles = json_array_get_count(files);

    int i;
    pthread_mutex_lock(&filesAccess);
    for (i = 0; i < numberOfFiles; i++)
    {
        //make block node and push it to the list
        JSON_Value* tmp = json_array_get_value(files,i);
        fileT *file = (fileT*)malloc(sizeof(fileT));
        strcpy(file->fileName, json_object_get_string(json_value_get_object(tmp), "fileName"));
        file->size = (unsigned int)json_object_get_number(json_value_get_object(tmp),"size");
        strcpy(file->checksum, json_object_get_string(json_value_get_object(tmp), "checkSum"));
        file->numberOfBlocks = (unsigned int)json_object_get_number(json_value_get_object(tmp),"numOfBlocks");

        list_node_t *findNode = NULL;
        if(globalFilesList->head!=NULL) findNode=list_find(globalFilesList, file);
        if(findNode==NULL)// check if file information already exists in the global files list
        {
            list_t *blocks = list_new();
            file->blocks = blocks;
            if(MakeBlocksList(file))
            {
                free(file);
                json_value_free(rootValue);
                return 1;
            }
            file->isComplete = (file->blocks->len) == (file->numberOfBlocks)  ? 1 : 0;
            list_node_t* fileNode = list_node_new(file);
            list_rpush(globalFilesList, fileNode);
        }
        else
        {
            free(file);
            if(MakeBlocksList(findNode->val))
            {
                json_value_free(rootValue);
                return 1;
            }
            fileT* foundFile = (fileT*) findNode->val;
            foundFile->isComplete = (foundFile->blocks->len) == (foundFile->numberOfBlocks)  ? 1 : 0;
        }
    }
    pthread_mutex_unlock(&filesAccess);
    json_value_free(rootValue);

    return 0;
}



char *GenerateFilesListMessage()
{
	JSON_Value *rootValue = json_value_init_object();
	JSON_Object *rootObject = json_value_get_object(rootValue);
	char *serializedString = NULL;

    JSON_Value *filesArray = json_value_init_array();
    json_object_set_value(rootObject, "files", filesArray);
    JSON_Array *arrayObject = json_value_get_array(filesArray);

    int i;
    pthread_mutex_lock(&filesAccess);
    for(i = 0; i < globalFilesList->len; i++)
    {
        list_node_t* fileNode = list_at(globalFilesList, i);
        fileT *file = (fileT *) fileNode->val;
        if(file->isComplete)
        {
            JSON_Value* tmp = json_value_init_object();
            char* fileName = file->fileName;
            unsigned int size = file->size;
            unsigned int blocks = file->blocks->len;
            char* checkSum = file->checksum;

            json_object_set_string(json_value_get_object(tmp), "fileName", fileName);
            json_object_set_number(json_value_get_object(tmp), "size", size);
            json_object_set_number(json_value_get_object(tmp), "numOfBlocks", blocks);
            json_object_set_string(json_value_get_object(tmp), "checkSum", checkSum);

            json_array_append_value(arrayObject,tmp);
        }
    }
    pthread_mutex_unlock(&filesAccess);
	serializedString = json_serialize_to_string(rootValue);
	json_value_free(rootValue);

	return serializedString;
}


fileBlockT* GetBlockInfo(char* jsonFile)
{
    fileBlockT* retBlock = NULL;
    JSON_Value *rootValue  = json_parse_string(jsonFile);
    char fileName[512];
    strcpy(fileName,json_object_get_string(json_value_get_object(rootValue), "parentFileName"));
    unsigned int index = (unsigned int)json_object_get_number(json_value_get_object(rootValue),"index");

    json_value_free(rootValue);

    /*
        Search the global list for requested block
    */

    int i;
    pthread_mutex_lock(&blocksAccess);
    for(i = 0; i < globalBlocksList->len; i++)
    {
        list_node_t* blockNode = list_at(globalBlocksList,i);
        fileBlockT* block = (fileBlockT*)blockNode->val;
        if((strcmp(block->fileName,fileName)==0) && (block->index==index))
        {
            retBlock = block;
            break;
        }
    }
    pthread_mutex_unlock(&blocksAccess);

    return retBlock;
}


char* GetPeerInfo(fileBlockT* block)
{
    JSON_Value *rootValue = json_value_init_object();
	JSON_Object *rootObject = json_value_get_object(rootValue);
	char *serializedString = NULL;
    list_node_t* peerNode = NULL;
    int j = ATTEMPTS;

    /*
        Get a random peer from the block's peer list.
    */
    do
    {
        int i = rand()%(block->peers->len);
        peerNode = list_at(block->peers, i);
        if(peerNode==NULL)
        {
            j--;
        }
        else 
            break;
    } while(j!=0);
    
    if(j==0)
    {
        json_value_free(rootValue);
        return NULL;
    }
    peerT* peer = (peerT*) peerNode->val;

    json_object_set_string(rootObject, "IP", peer->ip);
    json_object_set_number(rootObject, "socket", peer->listeningSocket);

    serializedString = json_serialize_to_string(rootValue);
	json_value_free(rootValue);

    return serializedString;
}


char* BlockNotFoundMessage()
{
    JSON_Value *rootValue = json_value_init_object();
	JSON_Object *rootObject = json_value_get_object(rootValue);
	char *serializedString = NULL;

    json_object_set_string(rootObject, "IP", "");
    json_object_set_number(rootObject, "socket", 0);

    serializedString = json_serialize_to_string(rootValue);
	json_value_free(rootValue);

    return serializedString;
}

char* GenerateOOM()
{
    JSON_Value *rootValue = json_value_init_object();
	JSON_Object *rootObject = json_value_get_object(rootValue);
	char *serializedString = NULL;

    json_object_set_string(rootObject, "error", "Message too large!\n");

    serializedString = json_serialize_to_string(rootValue);
	json_value_free(rootValue);

    return serializedString;
}


int GetSocket(char* jsonFile)
{
    fileBlockT* retBlock = NULL;
    JSON_Value *rootValue  = json_parse_string(jsonFile);
    int sock = (unsigned int)json_object_get_number(json_value_get_object(rootValue),"listeningPort");

    json_value_free(rootValue);

    return sock;
}

#include "list_functions.h"

int MatchPeer(void *a, void *b)
{
    peerT* p1 = (peerT*)a;
    peerT* p2 = (peerT*)b;
    if(strcmp(p1->ip, p2->ip)==0 && (p1->listeningSocket==p2->listeningSocket))
        return 1;
    else
        return 0;
    
}

int MatchBlock(void *a, void *b)
{
    fileBlockT* p1 = (fileBlockT*)a;
    fileBlockT* p2 = (fileBlockT*)b;
    if(strcmp(p1->fileName, p2->fileName)==0 && (p1->index==p2->index))
        return 1;
    else
        return 0;
    
} 

int MatchFile(void *a, void *b)
{
    fileT* p1 = (fileT*)a;
    fileT* p2 = (fileT*)b;
    if(strcmp(p1->fileName, p2->fileName)==0)
        return 1;
    else
        return 0;
}


int RemoveFromGlobal(peerT* peer)
{
    list_node_t* peerNode = NULL;
    pthread_mutex_lock(&peersAccess);
    if(globalPeersList->head!=NULL)
    {
        peerNode = list_find(globalPeersList, peer);
        if(peerNode!=NULL)
        {
            list_remove(globalPeersList, peerNode);
        }
    }
    pthread_mutex_unlock(&peersAccess);

    return 0;

}


int RemoveFromBlocks(peerT* peer)
{
    list_node_t* peerNode = NULL;
    int i;
    pthread_mutex_lock(&blocksAccess);
    for(i = 0; i < globalBlocksList->len; i++)
    {
        list_node_t* blockNode = list_at(globalBlocksList,i);
        fileBlockT* block = (fileBlockT*) blockNode->val;
        if(block->peers->head!=NULL)
        {
            peerNode = list_find(block->peers, peer);
            if(peerNode!=NULL)
            {
                list_remove(block->peers, peerNode);
            }
        }
    }
    
    pthread_mutex_unlock(&blocksAccess);
    
    return 0;
}

int RemoveFromParentFile(fileBlockT* block)
{
    int i;
    pthread_mutex_lock(&filesAccess);
    for(i = 0; i < globalFilesList->len; i++)
    {
        list_node_t* fileNode = list_at(globalFilesList,i);
        fileT* file = (fileT*) fileNode->val;
        if(strcmp(block->fileName,file->fileName)==0)
        {
            list_node_t* foundNode = list_find(file->blocks, block);
            if(foundNode!=NULL)
            {
                list_remove(file->blocks, foundNode);
                file->isComplete = (file->blocks->len) == (file->numberOfBlocks) ? 1 : 0;
            }
            if(file->blocks->len==0)
            {
                list_remove(globalFilesList, fileNode);
                list_destroy(file->blocks);
                free(file);
            }
            break;
        }
    }
    pthread_mutex_unlock(&filesAccess);
    return 0;
}


int CheckBlocksList()
{
    int i;
    pthread_mutex_lock(&blocksAccess);
    for(i = 0; i < globalBlocksList->len; i++)
    {
        list_node_t* blockNode = list_at(globalBlocksList,i);
        fileBlockT* block = (fileBlockT*) blockNode->val;
        if(block->peers->len==0)
        {
            RemoveFromParentFile(block);

            list_remove(globalBlocksList, blockNode);
            list_destroy(block->peers);
            free(block);
            
            i--;
        }
    }
    pthread_mutex_unlock(&blocksAccess);
    return 0;
}



void* CleanUp(void* args)
{
    while (1)
    {
         if (sem_trywait(&semClean) == 0)
        {
            pthread_mutex_lock(&threadsAccess);
            list_node_t* trashNode = list_rpop(threadList);
            if(trashNode!=NULL)
            {
                clientT* trash = (clientT* )trashNode->val;
                pthread_join(*(trash->thread),NULL);
                free(trash->thread);
            }
            pthread_mutex_unlock(&threadsAccess);

            free(trashNode);
            pthread_mutex_lock(&clientsAccess);
            numberOfClients--;
            pthread_mutex_unlock(&clientsAccess);

            printf("removed from thread list.\n");
            fflush(stdout);
        }

        if(numberOfClients==0)
        {
            if (sem_trywait(&semCleanFinish) == 0)
            {
                break;
            }
        }
    }

    return 0;
}
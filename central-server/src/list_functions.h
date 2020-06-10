#ifndef LIST_FUNCTIONS_H
#define LIST_FUNCTIONS_H

#include "list.h"
#include "../includes.h"


/*
    Match function for peers list search.
*/
int MatchPeer(void *, void *);


/*
    Match function for blocks list search.
*/
int MatchBlock(void *, void *);


/*
    Match function for files list search.
*/
int MatchFile(void *, void *);



/*
    Remove disconnected peer from the global peers list.
*/
int RemoveFromGlobal(peerT*);



/*
    For every block in global blocks list, remove disconnected
    peer from the block's peer list (if it was a part of the list).
*/
int RemoveFromBlocks(peerT*);



/*
    Check block lists to see if peer list is empty to remove
    the block from the global blocks list.
    Calls the check for global file list.
    
*/
int CheckBlocksList();



/*
    Check the global file list to remove that block from
    it's parent file blocks list and mark the file as incomplete.
    If that file's blocks list is empty, remove the file from the global
    files list.
*/
int RemoveFromParentFile(fileBlockT*);

/*
    Thread cleanup after the client has disconnected.
    The thread is terminated when it receives the finish signal.
*/

void* CleanUp(void*);


#endif //LIST_FUNCTIONS_H
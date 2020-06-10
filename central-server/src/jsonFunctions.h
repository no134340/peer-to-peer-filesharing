//
//
//

#ifndef CENTRAL_SERVER_JSONFUNCTIONS_H
#define CENTRAL_SERVER_JSONFUNCTIONS_H

#include "../includes.h"
#include "../include/parson.h"


/*
    Get client command. Returns command code or -1 if an error happens.
*/
int GetCommand(char*);


/*
    Read peer's listening socket info and 
    add peer to global peers list.
*/
int AddPeer(char *, peerT* );



/*
    Processes peer's block information and adds peer to the global peer list.
*/
int GetBlocks(char*, peerT*);


/*
    Add blocks to the file's blocks list.
*/
int MakeBlocksList(fileT*);

/*
    Add client's files info to the global files list.
*/
int GetFiles(char*);


/*
    Generate ls fileName message.
    Checks if a file in the global files list is complete, if it is,
    it adds it to the client message.
*/
char *GenerateFilesListMessage();


/*
    Get block based on peer request.
    Not tested yet. <- to be tested next
*/
fileBlockT* GetBlockInfo(char*);


/*
    Get a peer form the block's peer list.
    Not tested yet.
*/
char* GetPeerInfo(fileBlockT*);


/*
    Generates the block not found message.
*/
char* BlockNotFoundMessage();


/*
    Generates the message too large error.
*/
char* GenerateOOM();

int GetSocket(char*);

#endif //CENTRAL_SERVER_JSONFUNCTIONS_H

//
// Created by dzuda11 on 7.12.19..
//

#ifndef P2P_CLIENT_TRASHQUEUE_H
#define P2P_CLIENT_TRASHQUEUE_H

#include <pthread.h>
#include <sys/socket.h>

typedef struct SubThreadArg{
	struct sockaddr_in* client;
	int* client_sock;
	pthread_t* threadPointer;
	struct SubThreadArg* selfPointer;
}SubThreadArg;

struct Element
{
	struct SubThreadArg* pTrash;
	struct Element* next;
};

void initTrashQueue();
SubThreadArg* popTrash();
int pushTrash(SubThreadArg*);


#endif //P2P_CLIENT_TRASHQUEUE_H

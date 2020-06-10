//
// Created by dzuda11 on 7.12.19..
//

#include <stdio.h>
#include <stdlib.h>

#include "TrashQueue.h"
static struct Element* head;
static struct Element* tail;

static pthread_mutex_t trashMutex;

void initTrashQueue()
{
	static char inited = 'n';  // This allow only one initialization of trashQueue
	if (inited == 'n')
	{
		head = NULL;
		tail = NULL;
		pthread_mutex_init(&trashMutex, NULL);
		inited = 'y';
	}
}

SubThreadArg* popTrash()
{
	struct Element* tmp = NULL;
	SubThreadArg* ret = NULL; // return value

	pthread_mutex_lock(&trashMutex);
	if (head == NULL)
	{
		ret = NULL;
	}
	else
	{
		tmp = head;
		ret = head -> pTrash;
		head = head -> next;
	}
	pthread_mutex_unlock(&trashMutex);

	free(tmp);
	return ret;
}
int pushTrash(SubThreadArg* trash)
{
	struct Element* tmp = (struct Element*) malloc(sizeof(struct Element));
	if(tmp == NULL)
	{
		printf("[TRASH QUEUE] Failed to add to list. Element on address: %p", trash);
		return -1;
	}
	tmp -> pTrash = trash;
	tmp -> next = NULL;
	pthread_mutex_lock(&trashMutex);
	if(head == NULL)
	{
		head = tmp;
		tail = head;
	}
	else
	{
		tail -> next = tmp;
		tail = tail -> next;
	}
	pthread_mutex_unlock(&trashMutex);
	return 1;
}
//
// Created by dzuda11 on 7.12.19..
//

#ifndef P2P_CLIENT_SERVER_THREAD_H
#define P2P_CLIENT_SERVER_THREAD_H

#include <stdio.h>
#include <stdlib.h>
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write

void* ServerThread(void* arg);

#endif //P2P_CLIENT_SERVER_THREAD_H

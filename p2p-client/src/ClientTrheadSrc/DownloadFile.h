//
// Created by dzuda11 on 3.1.20..
//

#ifndef P2P_CLIENT_DOWNLOADFILE_H
#define P2P_CLIENT_DOWNLOADFILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../includes.h"

void DownloadFile(char *fileName, const char *downloadDir, unsigned int size, unsigned int numOfBlocks, const char *checkSum,
			 in_addr_t serverIP, int port);

#endif //P2P_CLIENT_DOWNLOADFILE_H

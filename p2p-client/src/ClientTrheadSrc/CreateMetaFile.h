//
// Created by dzuda11 on 14.1.20..
//

#ifndef P2P_CLIENT_CREATEMETAFILE_H
#define P2P_CLIENT_CREATEMETAFILE_H

#include <string.h>
#include "../includes.h"
#include <stdio.h>
#include <stdlib.h>
#include "../../include/parson.h"
#include "../sha256sum.h"


void CreateMetaFile(char* pathToFile, unsigned int numOfBlocks);

#endif //P2P_CLIENT_CREATEMETAFILE_H

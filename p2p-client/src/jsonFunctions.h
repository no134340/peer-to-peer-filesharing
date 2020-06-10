//
// Created by dzuda11 on 3.12.19..
//

#ifndef P2P_CLIENT_JSONFUNCTIONS_H
#define P2P_CLIENT_JSONFUNCTIONS_H

#include <stdio.h>
#include <dirent.h>
#include "includes.h"
#include "../include/parson.h"


char *GenerateC2SHelloMessage();

//JSON_Object* getFileInfo(char *file);

int getBlockInfo(int *, int *, char**, char*);
#endif //P2P_CLIENT_JSONFUNCTIONS_H

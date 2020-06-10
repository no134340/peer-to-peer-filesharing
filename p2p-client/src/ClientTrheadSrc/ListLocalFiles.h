//
// Created by dzuda11 on 8.12.19..
//

#ifndef P2P_CLIENT_LISTLOCALFILES_H
#define P2P_CLIENT_LISTLOCALFILES_H

#include <stdio.h>
#include "../includes.h"
#include "../../include/list.h"

int FillListWithFiles(list_t *fileList, const char* dir);
void printFileList(list_t *fileList);

#endif //P2P_CLIENT_LISTLOCALFILES_H

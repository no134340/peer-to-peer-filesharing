//
// Created by dzuda11 on 14.1.20..
//

#include "sha256sum.h"

void sha256sum(const char* pathToFile, char* sha256String)
{
	char* command = malloc(strlen("sha256sum ") + strlen(pathToFile) + strlen(" >> mile"));
	strcpy(command, "sha256sum ");
	strcat(command, pathToFile);
	strcat(command, " >> mile");

	if (system(command) != 0)
	{
		printf("sha256sum error\n");
		free(command);
		return;
	}

	FILE* mile = fopen("mile", "r");
	fscanf(mile, "%s", sha256String);
	fclose(mile);
	free(command);
	remove("mile");
}

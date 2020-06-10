//
// Created by dzuda11 on 8.12.19..
//

#include <dirent.h>
#include <string.h>
#include "ListLocalFiles.h"


static const int PART_WITHOUT_META = 0;
static const int PART_WITH_META = 1;
static const int FILE_WITHOUT_META = 2;
static const int FILE_WITH_META = 3;
static const int META_ONLY = 4;


struct MyFile
{
	char name[256];
	int state;
};

static void addMetaFile(list_t *list, const char *name);

static void addPartFile(list_t *list, const char *name);

static void addRegularFile(list_t *list, const char *name);


static int LocalFilesListCompare(void *x, void *y)
{
	if (strcmp(x, y) == 0)
		return 1;
	return 0;
}


int FillListWithFiles(list_t *fileList, const char *dir)
{
	struct dirent *de;
	DIR *dr = opendir(dir);

	if (dr == NULL)
	{
		printf("[ListLocalFiles] Could not open downloads directory");
		return -1;
	}

	fileList->match = LocalFilesListCompare;

	while ((de = readdir(dr)) != NULL)
	{
		if (de->d_type == DT_REG)
		{
			if (strncmp(de->d_name + strlen(de->d_name) - strlen(META_FILE_EXTENSION), META_FILE_EXTENSION, strlen(META_FILE_EXTENSION)) == 0)
			{
				// Handle meta files
				addMetaFile(fileList, de->d_name);

			} else if (strncmp(de->d_name + strlen(de->d_name) - strlen(PART_FILE_EXTENSION), PART_FILE_EXTENSION,
							   strlen(PART_FILE_EXTENSION)) == 0)
			{
				// Handle part files
				addPartFile(fileList, de->d_name);
			} else
			{
				// Handle regular files
				addRegularFile(fileList, de->d_name);
			}
		}
	}
}

void printFileList(list_t *fileList)
{
	int index = 0;
	list_node_t *node;
	list_iterator_t *it;

	it = list_iterator_new(fileList, LIST_HEAD);
	puts("PART FILES WITHOUT META FILE - this files can't be shared or continue their download");
	while ((node = list_iterator_next(it)))
	{
		struct MyFile *file = (struct MyFile *) node->val;
		if (file->state == PART_WITHOUT_META)
		{
			printf("\t%d\t%s\n", ++index, file->name);
			//list_remove(fileList, node);
		}
	}
	list_iterator_destroy(it);

	index = 0;
	it = list_iterator_new(fileList, LIST_HEAD);
	puts("\nPART FILES");
	while ((node = list_iterator_next(it)))
	{
		struct MyFile *file = (struct MyFile *) node->val;
		if (file->state == PART_WITH_META)
		{
			printf("\t%d\t%s\n", ++index, file->name);
			//list_remove(fileList, node);
		}
	}
	list_iterator_destroy(it);

	index = 0;
	it = list_iterator_new(fileList, LIST_HEAD);
	puts("\nMETA FILES WITHOUT FILES - this files should be removed");
	while ((node = list_iterator_next(it)))
	{
		struct MyFile *file = (struct MyFile *) node->val;
		if (file->state == META_ONLY)
		{
			printf("\t%d\t%s\n", ++index, file->name);
			//list_remove(fileList, node);
		}
	}
	list_iterator_destroy(it);

	index = 0;
	it = list_iterator_new(fileList, LIST_HEAD);
	puts("\nFILES WITHOUT META FILE - create meta files using 'create fileName'");
	while ((node = list_iterator_next(it)))
	{
		struct MyFile *file = (struct MyFile *) node->val;
		if (file->state == FILE_WITHOUT_META)
		{
			printf("\t%d\t%s\n", ++index, file->name);
			//list_remove(fileList, node);
		}
	}
	list_iterator_destroy(it);

	it = list_iterator_new(fileList, LIST_HEAD);
	puts("\nCOMPLETED FILES");
	while ((node = list_iterator_next(it)))
	{
		struct MyFile *file = (struct MyFile *) node->val;
		if (file->state == FILE_WITH_META)
		{
			printf("\t%d\t%s\n", ++index, file->name);
			//list_remove(fileList, node);
		}
	}
	list_iterator_destroy(it);
}

static void addMetaFile(list_t *fileList, const char *name)
{
	struct MyFile *file = (struct MyFile *) malloc(sizeof(struct MyFile));
	strncpy(file->name, name, strlen(name) - strlen(META_FILE_EXTENSION));
	list_node_t *tmp = list_find(fileList, file);
	if (tmp)
	{
		free(file);
		file = tmp->val;

		if(file->state == PART_WITHOUT_META)
		{
			file->state = PART_WITH_META;
		}
		else if(file->state == FILE_WITHOUT_META)
		{
			file->state = FILE_WITH_META;
		}
	} else
	{
		file->state = META_ONLY;
		list_rpush(fileList, list_node_new((void *) file));
	}
}

static void addPartFile(list_t *fileList, const char *name)
{
	struct MyFile *file = (struct MyFile *) malloc(sizeof(struct MyFile));
	*strrchr(name, '.') = 0;	// Find last '.' and put '\0' on position, this will remove .part extension
	strcpy(file->name, name);

	list_node_t *tmp = list_find(fileList, file);
	if (tmp)
	{
		free(file);
		file = tmp -> val;
		if(file->state == META_ONLY)
		{
			file->state = PART_WITH_META;
			// strcpy(file->name, name);
		}
	} else
	{
		//strcpy(file->name, name);
		file->state = PART_WITHOUT_META;
		list_rpush(fileList, list_node_new((void *) file));
	}
}

static void addRegularFile(list_t *fileList, const char *name)
{
	struct MyFile *file = (struct MyFile *) malloc(sizeof(struct MyFile));
	strcpy(file->name, name);
	//*strchr(name, '.') = 0;
	list_node_t *tmp = list_find(fileList, file);
	if (tmp)
	{
		free(file);
		file = tmp -> val;
		if(file->state == META_ONLY)
		{
			file->state = FILE_WITH_META;
			//strcpy(file->name, name);
		}
	} else
	{
		//strcpy(file->name, name);
		file->state = FILE_WITHOUT_META;
		list_rpush(fileList, list_node_new((void *) file));
	}
}

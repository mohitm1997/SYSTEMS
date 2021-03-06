#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

char path[4096];

typedef struct FileEntry{
	char* fileName;
	int count;
} FileEntry;

typedef struct IndexEntry{
	char* token;
	int size;
	int capacity;
	FileEntry* files;
} IndexEntry;

typedef struct Index{
	IndexEntry* entries;
	int capacity;
	int size;
} Index;

void printXML(FILE* f,Index* index){
	fprintf(f,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<fileIndex>\n");
	int i;
	int j;
	IndexEntry* indEnt;
	FileEntry* fe;
	for (i=0;i<index->size;i++){
		indEnt=&(index->entries[i]);
		fprintf(f,"\t<word text=\"%s\">\n",indEnt->token);
		for (j=0;j<indEnt->size;j++){
			fe=&(indEnt->files[j]);
			fprintf(f,"\t\t<file name=\"%s\">%i</file>\n",fe->fileName,fe->count);
		}
		fprintf(f,"\t</word>\n");
	}
	fprintf(f,"</fileIndex>\n");
}

Index* buildIndex(){
	Index* i;
	i = (Index*)malloc(sizeof(Index));
	i->entries= (IndexEntry*) malloc(sizeof(IndexEntry)*10);
	i->capacity=10;
	i->size = 0;
	return i;
}

IndexEntry* buildIndexEntry(char* token){
	IndexEntry* ie;
	ie = (IndexEntry*)malloc(sizeof(IndexEntry));
	ie->files=(FileEntry*) malloc(sizeof(FileEntry)*10);
	ie->capacity=10;
	ie->size=0;
	ie->token=token;
	return ie;
}

FileEntry* buildFileEntry(char* fileName){
		FileEntry* fe;
		fe = (FileEntry*)malloc(sizeof(FileEntry));
		fe->fileName=fileName;
		fe->count=0;
		return fe;
}

//check if file entry exists for current index entry, if it does, increment, if not, build new entry and add to index.  
void addFileEntry(IndexEntry* toAdd, char* fileName){
	int i;
	FileEntry* fe = NULL;
	for(i=0;i<toAdd->size;i++){
		if (!strcmp(toAdd->files[i].fileName,fileName)){
			fe=&(toAdd->files[i]);
		}
	}

	if(fe == NULL){
		fe = buildFileEntry(fileName);
		toAdd->size+=1;
		if (toAdd->size>=toAdd->capacity){
			toAdd->files = realloc(toAdd->files,toAdd->capacity*2);
			toAdd->capacity*=2;
			printf("fileEntry expand");
		}
		toAdd->files[toAdd->size-1]=*fe;
	}
	fe->count+=1;
	printf("file entry file name: %s\n",fe->fileName );
	printf("index entry file entry fname= %s\n", toAdd->files[toAdd->size-1].fileName);
}
 
void addIndexEntry(Index* toAdd, char* token, char* fname){
	int i;
	IndexEntry* entry= NULL;
	//check if index entry exists for this token 
	for (i=0;i<toAdd->size;i++){
		if (!strcmp(toAdd->entries[i].token,token)){
			entry=&(toAdd->entries[i]);
		}
	}
	//if entry does not exist for token in index, create new indexEntry for that token, 
	if (entry == NULL){
	entry = buildIndexEntry(token);
	toAdd->size+=1;
	if (toAdd->size>=toAdd->capacity){
		printf("index entry expand");
		toAdd->entries = realloc(toAdd->entries,toAdd->capacity*2);
		toAdd->capacity*=2;
	}
	toAdd->entries[toAdd->size-1]=*entry;
	}
	
	//add file entry for this file or update existing. 

		addFileEntry(entry,fname);	
}



int isFile(const char* path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

void tokenize(int fd, char* fname, Index* index)
{
	char* token = NULL;
	int i = 0;
	char buffer[1];
	while(read(fd,buffer,1) != 0)
	{
		while(isalnum(buffer[0]))
		{
			if(token == NULL)
			{
				token = (char*)malloc(2);
				token[0] = buffer[0];
			}
			else
			{
				char* temp = (char*)realloc(token,i+2);
				token = temp;
				token[i] = buffer[0];
			}
			
			i++;
			if(read(fd,buffer,1) == 0) break;
		}

		if(token != NULL)
		{
			token[i] = '\0';
			//printf("%s\n", token);
			addIndexEntry(index,token,fname);
			token = NULL;
			i = 0;
		}
	}
	return;
}



void indexDir(char name[4096], Index* index)
{
				printXML(stdout,index);

	struct dirent * currObj = NULL;
	DIR* directory = opendir(name);
	currObj = readdir(directory);

	while(currObj != NULL )
	{
		if (currObj->d_name[0]=='.')
		{
			currObj = readdir(directory);
			continue;
		}

		if(currObj->d_type == DT_REG)
		{
			//currObj is a file
			//printf("\tfilename: %s\n",currObj->d_name);
			char* temp = (char*)malloc(strlen(name) + strlen(currObj->d_name) + 2);
			strcpy(temp,name);
			strcat(temp,"/");
			strcat(temp,currObj->d_name);
			printf("%s\n",temp);		
			int fd = open(temp,O_RDONLY);
			tokenize(fd, currObj->d_name, index);
			free(temp);
			close(fd);
		}
		
		else if(currObj->d_type == DT_DIR)
		{	
			//printf("\tdirname: %s\n",currObj->d_name);

			char* temp = (char*)malloc(strlen(name) + strlen(currObj->d_name) + 2);
			strcpy(temp,name);
			strcat(temp,"/");
			strcat(temp,currObj->d_name);
			indexDir(temp,index);
			
			free(temp);
		}

		else
		{
			//other
		}
		
		if ((currObj = readdir(directory)) == NULL){
			break;}

	}

	return;	
}



int main(int argc, char** argv)
{
	if(argc != 3)
	{
		fprintf(stderr, "ERROR: Invalid number of arguments\nUSAGE: ./invertedindex <inverted-index file name> <directory or file name>\n");
		return 1;
	}

	//FILE* output = fopen(argv[1],"w");

	//if 2nd argument is a file

	Index* i = buildIndex();
	//FILE* out = fopen(argv[1],"w");
	if(isFile(argv[2]))
	{
		int fd = open(argv[2],O_RDONLY);
		tokenize(fd, argv[2],i);
		close(fd);
	}

	//if 2nd argument is a directory(will handle recursive directory traversal after program is working with a single file)
	else
	{

		strcpy(path,"./");
		strcat(path,argv[2]);
		indexDir(path, i);
	}
	//printXML(stdout,i);
	return 0;
}
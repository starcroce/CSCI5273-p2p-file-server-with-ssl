#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#define MAXBUFFSIZE 1000
#define MAX 30

typedef struct{
	char fileName[128];
	int fileSize;
	char fileOwner[8];
	char ownerIP[128];
	int ownerPort;
} FileInfo;


typedef struct 
{
    char names[MAX][8];
    char num;
} NameList;

typedef struct{
	int num;
    char owner[8];
	FileInfo files[MAX];
} FileList;

// type = 1 for file list, type = 0 for command
typedef struct {
    int type;
    char cmd[256];
    FileList fileList;
} Packet;


// client get local files and construct local file list
void getFileList(FileList *fileList){
	struct dirent **namelist;
	int n;
	n = scandir(".", &namelist, 0, alphasort);
	if(n < 0){
		perror("scan dir");
		exit(1);
	}
    fileList->num = 0;
	while(n--)
    {
		if((strcmp(namelist[n]->d_name, "..") != 0) && (strcmp(namelist[n]->d_name, ".") != 0 ))
        {
            strcpy(fileList->files[fileList->num].fileName, namelist[n]->d_name);
            fileList->num++;
		}
		free(namelist[n]);
	}
	free(namelist);
}

void printFileList(FileList *fileList){
	int i;
	for(i = 0; i < fileList->num; i++){
		printf("%s||%d||%s||%s||%d\n",
               fileList->files[i].fileName,
               fileList->files[i].fileSize,
               fileList->files[i].fileOwner,
               fileList->files[i].ownerIP,
               fileList->files[i].ownerPort);
	}
}

void copyFileList(FileList *copyto, FileList *from)
{
    int i;
    copyto->num = from->num;
    for (i = 0; i < from->num; i++)
    {
        strcpy(copyto->files[i].fileName, from->files[i].fileName);
        strcpy(copyto->files[i].fileOwner, from->files[i].fileOwner);
        copyto->files[i].fileSize = from->files[i].fileSize;
        strcpy(copyto->files[i].ownerIP, from->files[i].ownerIP);
        copyto->files[i].ownerPort = from->files[i].ownerPort;
    }
}

// merge origin master file list with new file list
void mergeFileList(FileList *master, FileList *newList){
	int i;
    int size = master->num;
	for(i = size; i < size + newList->num; i++)
    {
        strcpy(master->files[i].fileName, newList->files[i - size].fileName);
        strcpy(master->files[i].fileOwner, newList->files[i - size].fileOwner);
        master->files[i].fileSize = newList->files[i - size].fileSize;
        strcpy(master->files[i].ownerIP, newList->files[i - size].ownerIP);
        master->files[i].ownerPort = newList->files[i - size].ownerPort;
		//master->fileList[i] = newList->fileList[i - master->num];
	}
    master->num = size + newList->num;
}

// check if client already existed
// 0 for new client, 1 for already existed
int isExisted(NameList *clients, char *name)
{
    int flag = 0;
    int i;
    for(i = 0; i < MAX; i++)
    {
        if(strcmp(clients->names[i], name) == 0)
        {
            flag = 1;
            return flag;
        }
    }
    strcpy(clients->names[clients->num], name);
    clients->num++;
    return flag;
}

// use select to check if there is keyboard input
// 0 for no input, non-zero for input
int kbhit()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

// remove exit clinet's file list from master file list
void deregisterClient(NameList *clients, FileList *master, char *name)
{   
    // remove client name from name list
    int i = 0;
    while(strcmp(clients->names[i], name) != 0)
    {
        i++;
    }
    int j;
    for(j = i; j < clients->num; j++)
    {
        strcpy(clients->names[j], clients->names[j+1]);
    }
    clients->num--;

    // remove client's file list from master file list
    // get the position of exit client in the master file list
    int pos1 = 0, pos2 = 0;
    for(i = 0; i < master->num; i++)
    {
        if(strcmp(master->files[i].fileOwner, name) == 0)
        {
            pos1 = i;
            pos2 = pos1;
            while(strcmp(master->files[pos2].fileOwner, name) == 0)
            {
                pos2++;
            }
            break;
        }
    }
    // overwrite these entries with the following entries
    int range = pos2 - pos1;
    master->num -= range;
    for(i = pos1; i < master->num; i++)
    {
        if(strcmp(master->files[i].fileOwner, name) == 0)
        {
            strcpy(master->files[i].fileName, master->files[i + range].fileName);
            strcpy(master->files[i].fileOwner, master->files[i + range].fileOwner);
            master->files[i].fileSize = master->files[i + range].fileSize;
            strcpy(master->files[i].ownerIP, master->files[i + range].ownerIP);
            master->files[i].ownerPort = master->files[i + range].ownerPort;
        }
    }
}
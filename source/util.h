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
#define MAX 10

typedef struct{
	char fileName[128];
	int fileSize;
	char fileOwner[8];
	char ownerIP[128];
	int ownerPort;
} FileInfo;

typedef struct{
	int num;
	FileInfo files[MAX];
} FileList;

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
	fileList->num = n - 2;
	int k = 0;
	while(n--){
		FileInfo fileInfo;
		if((strcmp(namelist[n]->d_name, "..") != 0) && (strcmp(namelist[n]->d_name, ".") != 0 )){
			fileInfo.fileName = namelist[n]->d_name;
			fileList->files[k] = fileInfo;
			k++;
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

// merge origin master file list with new file list
FileList *mergeFileList(FileList *master, FileList *newList){
	int i;
	for(i = master->num; i < master->num + newList->num; i++){
		master->fileList[i] = newList->fileList[i - master->num];
	}
	return master;
}


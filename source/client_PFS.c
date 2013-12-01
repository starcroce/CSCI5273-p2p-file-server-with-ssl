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
	char *fileName;
	int fileSize;
	char *fileOwner;
	char *ownerIP;
	int ownerPort;
} FileInfo;

typedef struct{
	int num;
	FileInfo files[MAX];
} FileList;


// get file name list
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



// ./client_PFS <Client Name> <Server IP> <Server Port> <Private Key> <Certificate of this Client> <CA Cert>
int main(int argc, char const *argv[]){
	struct sockaddr_in clieAddr, p2pAddr;
	int clieSock; // for connecting to server
	int p2pSock; // for p2p transfer

	// setup sockaddr
	bzero(&clieAddr, sizeof(clieAddr));
	clieAddr.sin_family = AF_INET;
	clieAddr.sin_addr.s_addr = inet_addr(argv[2]);
	clieAddr.sin_port = htons(atoi(argv[3]));

	bzero(&p2pAddr, sizeof(p2pAddr));
	p2pAddr.sin_family = AF_INET;
	p2pAddr.sin_addr.s_addr = INADDR_ANY;
	p2pAddr.sin_port = htons(9500 + (argv[1] - 'A'));

	// create socket
	clieSock = socket(AF_INET, SOCK_STREAM, 0);
	p2pSock = socket(AF_INET, SOCK_STREAM, 0);
	if(clieSock < 0 || p2pSock < 0){
		perror("socket creation");
		exit(1);
	}

	// bind socket with p2p port
	if(bind(p2pSock, (struct sockaddr *)&p2pAddr, sizeof(p2pAddr)) < 0){
		perror("bind socket");
		exit(1);
	}

	// listen on the p2p port
	if(listen(p2pSock, MAX) < 0){
		perror("listen on the port");
		exit(1);
	}

	// connect to server
	if(connect(clieSock, (struct sockaddr *)&clieAddr, sizeof(clieAddr)) < 0){
		perror("connect to server");
		exit(1);
	}

	// get local file info list
	FileList fileList;
	struct stat st;
	getFileList(&fileList);
	int i;
	for(i = 0; i < fileList.num; i++){
		stat(fileList.files[i].fileName, &st);
		fileList.files[i].fileSize = st.st_size;
		fileList.files[i].fileOwner = argv[1];
		fileList.files[i].ownerIP = "local";
		fileList.files[i].ownerPort = 9500 + (argv[1][0] - 'A');
	}
	printFileList(&fileList);

	// upload file list to server
	int nbytes;
	nbytes = send(clieSock, &fileList, sizeof(FileList), 0);
	if(nbytes < 0){
		perror("init upload");
		exit(1);
	}

	char command[MAXBUFFSIZE];
	FileList recvFileList;
	while(1){
		printf("input command: ls, get, exit\n");
		gets(command);
		int commandLength = strlen(command);

		// ls command
		if(strstr(command, "ls")){
			nbytes = send(clieSock, command, commandLength, 0);
			if(nbytes < 0){
				perror("send ls command");
			}
			else if(nbytes > 0){
				nbytes = recv(clieSock, &recvFileList, sizeof(FileList), 0);
				if(nbytes < 0){
					perror("receive updated file list");
				}
				else if(nbytes > 0){
					printFileList(&recvFileList);
				}
			}
		}


	}



	return 0;
}
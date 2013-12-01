#include <sys/types.h>
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
#include <string.h>
#include <dirent.h>

#define MAXBUFFSIZE 1000
#define MAX 10

typedef struct{
	char *fileName;
	int fileSize;
	char fileOwner;
	char *ownerIP;
	int ownerPort;
} FileInfo;

typedef struct{
	int num;
	FileInfo fileList[MAX];
} FileList;

// merge origin master file list with new file list
FileList *mergeFileList(FileList *master, FileList *newList){
	int i;
	for(i = master->num; i < master->num + newList->num; i++){
		master->fileList[i] = newList->fileList[i - master->num];
	}
	return master;
}





// ./server_PFS <port number> <private key> <certificate of server> <CA sert>
int main(int argc, char const *argv[]){
	struct sockaddr_in servAddr;
	int servSock; // for listening
	int connectSock; // for connection

	// setup sockaddr
	bzero(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADOOR_ANY;
	servAddr.sin_port = htons(atoi(argv[1]));

	// create socket
	servSock = socket(AF_INET, SOCK_STREAM, 0);
	if(servSock < 0){
		perror("socket creation");
		exit(1);
	}

	// bind socket with server port
	if(bind(servSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0){
		perror("bind socket");
		exit(1);
	}

	// listen on the port
	if(listen(servSock, MAX) < 0){
		perror("listen on the port");
		exit(1);
	}

	// accept the connection
	connectSock = accept(servSock, NULL, sizeof(struct sockaddr_in));

	int nbytes;
	char command[MAXBUFFSIZE];

	// init master file list
	FileList masterFileList;
	FileList recvFileList;
	masterFileList->num = 0;

	while(1){
		// recv file list from new client
		nbytes = recv(connectSock, &recvFileList, sizeof(FileList), 0);
		if(nbytes < 0){
			perror("recv file list from client");
		}
		else if(nbytes > 0){
			masterFileList = mergeFileList(&masterFileList, &recvFileList);	
		}

		// multicast the updated file list









		bzero(command, sizeof(command));
		// recv the command from client
		nbytes = recv(connectSock, command, MAXBUFFSIZE, 0);
		if(nbytes > 0){
			printf("client: %s\n", command);
		}

		// ls command
		if(strcmp(command, "ls") == 0){
			// send the master file list
			nbytes = send(connectSock, &masterFileList, sizeof(FileList), 0);
			if(nbytes < 0){
				perror("ls fail");
			}
		}



		
	}


	return 0;
}
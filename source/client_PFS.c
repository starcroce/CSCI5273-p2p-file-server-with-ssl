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
#include "util.h"

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
	FileList localList;
    FileList masterList;
    Packet sendPacket;
    Packet recvPacket;
	
    struct stat st;
	getFileList(&(sendPacket.fileList));
	int i;
	for(i = 0; i < localList.num; i++){
		stat(sendPacket.fileList.files[i].fileName, &st);
		sendPacket.fileList.files[i].fileSize = st.st_size;
		strcpy(sendPacket.fileList.files[i].fileOwner, argv[1]);
		strcpy(sendPacket.fileList.files[i].ownerIP, "local");
		sendPacket.fileList.files[i].ownerPort = 9500 + (argv[1][0] - 'A');
	}
	printFileList(&localList);

	// upload file list to server
	int nbytes;
	nbytes = send(clieSock, &sendPacket, sizeof(Packet), 0);
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

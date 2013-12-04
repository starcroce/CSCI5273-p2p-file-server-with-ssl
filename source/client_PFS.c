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
#include <sys/fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include "util.h"

// ./client_PFS <Client Name> <Server IP> <Server Port> <Private Key> <Certificate of this Client> <CA Cert>
int main(int argc, char const *argv[]){
	struct sockaddr_in remoteAddr, p2pAddr;
	int clieSock; // for connecting to server
	int p2pSock; // for p2p transfer

	// setup sockaddr
	bzero(&remoteAddr, sizeof(remoteAddr));
	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_addr.s_addr = inet_addr(argv[2]);
	remoteAddr.sin_port = htons(atoi(argv[3]));

	bzero(&p2pAddr, sizeof(p2pAddr));
	p2pAddr.sin_family = AF_INET;
	p2pAddr.sin_addr.s_addr = INADDR_ANY;
	p2pAddr.sin_port = htons((int)(9500 + (argv[1] - 'A')));

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
	if(connect(clieSock, (struct sockaddr *)&remoteAddr, sizeof(remoteAddr)) < 0){
		perror("connect to server");
		exit(1);
	}
    // set sockets to non-block mode
    if (fcntl(clieSock, F_SETFL, O_NDELAY) < 0)
    {
        perror("set non-block");
        exit(1);
    }
    if (fcntl(p2pSock, F_SETFL, O_NDELAY) < 0)
    {
    }
	// get local file info list
	FileList localList;
    FileList masterList;
    Packet localFileListPacket;
    Packet sendCmdPacket;
    Packet recvPacket;
    localFileListPacket.type = 0;
    sendCmdPacket.type = 1;
	
    struct stat st;
	getFileList(&(localFileListPacket.fileList));
	int i;
	for(i = 0; i < localList.num; i++)
    {
		stat(localFileListPacket.fileList.files[i].fileName, &st);
		localFileListPacket.fileList.files[i].fileSize = st.st_size;
		strcpy(localFileListPacket.fileList.files[i].fileOwner, argv[1]);
		strcpy(localFileListPacket.fileList.files[i].ownerIP, "127.0.0.1");
		localFileListPacket.fileList.files[i].ownerPort = 9500 + (argv[1][0] - 'A');
	}
	printFileList(&(localFileListPacket.fileList));

	// upload file list to server
	int nbytes;
	nbytes = send(clieSock, &localFileListPacket, sizeof(Packet), 0);
	if(nbytes < 0){
		perror("init upload");
		exit(1);
	}

	char command[128];
    while(1)
    {
		printf("input command: ls, get, exit\n");
		gets(command);

		// ls command
		if(strcmp(command, "ls"))
        {
            strcpy(sendCmdPacket.cmd, command);
			nbytes = send(clieSock, &sendCmdPacket, sizeof(Packet), 0);
			if(nbytes < 0){
				perror("send ls command");
			}
        }
        if (strcmp(command, "exit"))
        {
        }
        if (strcmp(command, "get"))
        {
        }

        // try to receive from server
        nbytes = recv(clieSock, &recvPacket , sizeof(Packet), 0);
        if(nbytes < 0)
        {
        perror("receive updated file list");
        }
		else if(nbytes > 0){
			if (recvPacket.type == 0)
            {// command
            }
            else
            {// file list
                printFileList(&(recvPacket.fileList));
                // copy to master file list
                copyFileList(&(masterList), &(recvPacket.fileList));
            }
		}
    }





	return 0;
}

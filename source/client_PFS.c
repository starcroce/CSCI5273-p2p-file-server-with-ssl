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
	int clieSock;   // for connecting to server
	int p2pSock;    // for p2p transfer
    int toexit = 0;       // exit

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
    FileList masterList;
    Packet localFileListPacket;
    Packet sendCmdPacket;
    Packet recvPacket;
    localFileListPacket.type = 1;
    sendCmdPacket.type = 0;
	
    struct stat st;
	getFileList(&(localFileListPacket.fileList));
	strcpy(localFileListPacket.fileList.owner, argv[1]);
    strcpy(sendCmdPacket.fileList.owner, argv[1]);
	int i;
	for(i = 0; i < localFileListPacket.fileList.num; i++)
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

        while(!kbhit())
        {
            // no user input, recv from server
            nbytes = recv(clieSock, &recvPacket, sizeof(Packet), 0);
            if (nbytes > 0)
            {
                if (recvPacket.type == 0)
                {
                    if (strcmp(recvPacket.cmd, "Client existed")==0)
                    {
                        // client already existed, quit
                        printf("Client %s already registered with server, exiting...\n", argv[1]);
                        exit(1);
                    }
                }
                else
                {// recv file list
                    printf("Received master file list from server\n");
                    copyFileList(&(masterList), &(recvPacket.fileList));
                    printFileList(&(masterList));
                }
            }
            // try to recv from p2p address
        }
		gets(command);
        printf("get user input: %s\n", command);
		// ls command
		if(strcmp(command, "ls") == 0)
        {
            strcpy(sendCmdPacket.cmd, command);
			nbytes = send(clieSock, &sendCmdPacket, sizeof(Packet), 0);
			if(nbytes < 0)
            {
				perror("send ls command");
			}
        }
        if (strcmp(command, "exit") == 0)
        {
            // deregister with server
            strcpy(sendCmdPacket.cmd, command);
            nbytes = send(clieSock, &sendCmdPacket, sizeof(Packet), 0);
            if (nbytes < 0)
            {
                perror("send exit command");
            }
            else
            {
                printf("Client %s exiting...\n", argv[1]);
                exit(1);
            }
        }
        if (strstr(command, "get") == 0)
        {

        }
    }

	return 0;
}

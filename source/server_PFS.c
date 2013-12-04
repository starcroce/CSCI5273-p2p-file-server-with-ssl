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
#include <sys/fcntl.h>
#include "util.h"




// ./server_PFS <port number> <private key> <certificate of server> <CA sert>
int main(int argc, char const *argv[]){
	struct sockaddr_in servAddr;
	int servSock; // for listening
	int connectSocks[MAX]; // for connection
	NameList clients;
	clients.num = 0;
	// init connectSocks
	int i, j;
	for(i = 0; i < MAX; i++){
		connectSocks[i] = -1;
	}

	// setup sockaddr
	bzero(&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY;
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

	// set to non-block mode
	if(fcntl(servSock, F_SETFL, O_NDELAY) < 0){
		perror("set non-block");
		exit(1);
	}

	// listen on the port
	if(listen(servSock, MAX) < 0){
		perror("listen on the port");
		exit(1);
	}

	int nbytes;
	char command[MAXBUFFSIZE];
	int flag;

	// init master file list
	//FileList masterFileList;
	// FileList recvFileList;
	//masterFileList.num = 0;

	Packet sendFlieListPacket, sendCmdPacket;
	Packet recvPacket;

	sendCmdPacket.type = 0;
	recvPacket.type = 100;
	strcpy(sendCmdPacket.cmd, "Client existed");
	sendFlieListPacket.type = 1;
	sendFlieListPacket.fileList.num = 0;

	while(1)
	{

		// accept the client connection and set non-block
		for(i = 0; i < MAX; i++)
		{
			if(connectSocks[i] == -1)
			{
				connectSocks[i] = accept(servSock, NULL, sizeof(struct sockaddr_in));
				if(connectSocks[i] > 0)
				{
					// set connectSock to non-block
					if(fcntl(connectSocks[i], F_SETFL, O_NDELAY) < 0)
					{
						perror("cannot set connect sock non-block");
					}
					printf("client connected\n");
				}
			}
		}

		// recv file list or command from client, recv type 0 for command, 1 for file list
		for(i = 0; i < MAX; i++)
		{
			if(connectSocks[i] > 0)
			{
				nbytes = recv(connectSocks[i], &recvPacket, sizeof(Packet), 0);
				if(nbytes > 0)
				{
					printf("recv sth from client type %d\n", recvPacket.type);
					// if recv new file list
					if(recvPacket.type == 1)
					{
						// check if already client name already existed
						flag = isExisted(&clients, recvPacket.fileList.owner);
						printf("isExisted say %d\n", flag);
						// new client
						if(flag == 0)
						{
							printf("new client\n");
							mergeFileList(&sendFlieListPacket.fileList, &recvPacket.fileList);	
							for(j = 0; j < MAX; j++)
							{
								if(connectSocks[j] > 0)
								{
									nbytes = send(connectSocks[j], &sendFlieListPacket, sizeof(Packet), 0);
									if(nbytes < 0)
									{
										perror("push updated file list");
									}
									if(nbytes > 0)
									{
										printf("push the merged file list\n");
									}
								}
							}	
						}
						// client already existed
						if(flag == 1)
						{
							nbytes = send(connectSocks[i], &sendCmdPacket, sizeof(Packet), 0);
							if(nbytes < 0)
							{
								perror("send command");
							}
						}	
					}
					// if receive command from client
					if(recvPacket.type == 0)
					{
						printf("client: %s\n", recvPacket.cmd);
						// ls command
						if(strcmp(recvPacket.cmd, "ls") == 0)
						{
							nbytes = send(connectSocks[i], &sendFlieListPacket, sizeof(Packet), 0);
							if(nbytes < 0)
							{
								perror("response to ls command");
							}
						}
						// exit command
						if(strcmp(recvPacket.cmd, "exit") == 0)
						{
							// deregister client and push the updated file list
							deregisterClient(&clients, &sendFlieListPacket.fileList, recvPacket.fileList.owner);
							// close this socket
							close(connectSocks[i]);
							connectSocks[i] = -1;
							for(j = 0; j < MAX; j++)
							{
								if(connectSocks[j] > 0)
								{
									nbytes = send(connectSocks[j], &sendFlieListPacket, sizeof(Packet), 0);
									if(nbytes < 0)
									{
										perror("push updated file list");
									}
									if(nbytes > 0)
									{
										printf("push the updated file list after deregister\n");
									}
								}
							}
						}
					}
				}
			}
		}



	}


	return 0;
}

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

	// init connectSocks
	int i;
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
	FileList masterFileList;
	FileList recvFileList;
	masterFileList.num = 0;

	while(1){
		// accept the client connection and set non-block
		for(i = 0; i < MAX; i++){
			if(connectSocks[i] == -1){
				connectSocks[i] = accept(servSock, NULL, sizeof(struct sockaddr_in));
				if(connectSocks[i] > 0){
					// set connectSock to non-block
					if(fcntl(connectSocks[i], F_SETFL, O_NDELAY) < 0){
						perror("cannot set connect sock non-block");
					}
				}
			}
		}

		// recv file list or command from client
		for(i = 0; i < MAX; i++){
			if(connectSocks[i] > 0){
				// recv file list from new client
				nbytes = recv(connectSocks[i], &recvFileList, sizeof(FileList), 0);
				if(nbytes > 0){
					flag = 1;
					mergeFileList(&masterFileList, &recvFileList);	
				}
				// recv the command from client
				bzero(command, sizeof(command));
				nbytes = recv(connectSocks[i], command, MAXBUFFSIZE, 0);
				if(nbytes > 0){
					flag = 2;
					printf("client: %s\n", command);
					// if it is "ls", send the master file list
					if(strcmp(command, "ls") == 0){
						nbytes = send(connectSocks[i], &masterFileList, sizeof(FileList), 0);
						if(nbytes < 0){
							perror("ls, send master file list");
						}
					}
				}

			}
		}

		// multicast the updated file list
		if(flag == 1){
			for(i = 0; i < MAX; i++){
				if(connectSocks[i] > 0){
					nbytes = send(connectSocks[i], &masterFileList, sizeof(FileList), 0);
					if(nbytes < 0){
						perror("send master file list");
					}
				}
			}
		}


		// // receive commands
		// if(flag == 2){
		// 	// ls command
		// 	if(strcmp(command, "ls") == 0){
		// 		// send the master file list
		// 		nbytes = send(connectSock, &masterFileList, sizeof(FileList), 0);
		// 		if(nbytes < 0){
		// 			perror("ls fail");
		// 		}
		// 	}
		// }




	}


	return 0;
}

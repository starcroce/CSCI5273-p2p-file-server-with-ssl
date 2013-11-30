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
	p2pAddr.sin_addr.s_addr = INADOOR_ANY;
	p2pAddr.sin_port = htons(9500 + (argv[1] - 'A'));

	// create socket
	clieSock = socket(AF_INET, SOCK_STREAM, 0);
	p2pSock = socket(AF_INET, SOCK_STREAM, 0)
	if(servSock < 0 || p2pSock < 0){
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

	// upload file list to server



	while(1){

	}



	return 0;
}
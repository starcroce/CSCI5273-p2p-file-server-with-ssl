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
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>




// ./server_PFS <port number> <private key> <certificate of server> <CA sert>
int main(int argc, char const *argv[]){
	int i, j;
	// ssl setup
	SSL_CTX *ctx;
	SSL *ssl[MAX];
	SSL_METHOD *meth;
	X509 *clientCert[MAX];

	// Load encryption & hashing algorithms for the SSL program
	SSL_library_init();
	// Load the error strings for SSL & CRYPTO APIs
	SSL_load_error_strings();
	// Create a SSL_METHOD structure (choose a SSL/TLS protocol version)
	meth = SSLv3_method();
	// Create a SSL_CTX structure
	ctx = SSL_CTX_new(meth);
	if(!ctx)
	{
		ERR_print_errors_fp(stderr);
		exit(1);
	}
	// Load the server certificate into the SSL_CTX structure
	if(SSL_CTX_use_certificate_file(ctx, argv[3], SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		exit(1);
	}
	// Load the private-key corresponding to the server certificate
	if(SSL_CTX_use_PrivateKey_file(ctx, argv[2], SSL_FILETYPE_PEM) <= 0)
	{
		ERR_print_errors_fp(stderr);
		exit(1);
	}
	// Check if the server certificate and private-key matches
	if(!SSL_CTX_check_private_key(ctx))
	{
		printf("Private key does not match the certificate public key\n");
		exit(1);
	}
	// Load the RSA CA certificate into the SSL_CTX structure
	if(!SSL_CTX_load_verify_locations(ctx, argv[4], NULL))
	{
		ERR_print_errors_fp(stderr);
		exit(1);
	}
	// Set to require peer (client) certificate verification
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
	// Set the verification depth to 1
	SSL_CTX_set_verify_depth(ctx, 1);

	// create SSL structure
	// for(i = 0; i < MAX; i++)
	// {
	// 	ssl[i] = SSL_new(ctx);
	// }





	struct sockaddr_in servAddr;
	int servSock; // for listening
	int connectSocks[MAX]; // for connection
	NameList clients;
	clients.num = 0;
	// init connectSocks
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
					// // set connectSock to non-block
					// if(fcntl(connectSocks[i], F_SETFL, O_NDELAY) < 0)
					// {
					// 	perror("cannot set connect sock non-block");
					// }
					printf("client connected\n");
					ssl[i] = SSL_new(ctx);

					// Assign the socket into the SSL structure
					SSL_set_fd(ssl[i], connectSocks[i]);
					// Perform SSL Handshake on the SSL server
					nbytes = SSL_accept(ssl[i]);
					printf("%d\n", nbytes);
					if(nbytes == 1)
					{
						printf("ssl connected to client\n");
					}
					if(nbytes <= 0)
					{
						SSL_get_error(ssl[i], nbytes);
					}
					// // Get the client's certificate
					// clientCert[i] = SSL_get_peer_certificate(ssl[i]);
					// set connectSock to non-block
					if(fcntl(connectSocks[i], F_SETFL, O_NDELAY) < 0)
					{
						perror("cannot set connect sock non-block");
					}
				}
			}
		}

		// recv file list or command from client, recv type 0 for command, 1 for file list
		for(i = 0; i < MAX; i++)
		{
			if(connectSocks[i] > 0)
			{
				// nbytes = recv(connectSocks[i], &recvPacket, sizeof(Packet), 0);
				nbytes = SSL_read(ssl[i], &recvPacket, sizeof(Packet));
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
									// nbytes = send(connectSocks[j], &sendFlieListPacket, sizeof(Packet), 0);
									nbytes = SSL_write(ssl[j], &sendFlieListPacket, sizeof(Packet));
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
							// nbytes = send(connectSocks[i], &sendCmdPacket, sizeof(Packet), 0);
							nbytes = SSL_write(ssl[i], &sendCmdPacket, sizeof(Packet));
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
							// nbytes = send(connectSocks[i], &sendFlieListPacket, sizeof(Packet), 0);
							nbytes = SSL_write(ssl[i], &sendFlieListPacket, sizeof(Packet));
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
							SSL_shutdown(ssl[i]);
							close(connectSocks[i]);
							SSL_free(ssl[i]);
							connectSocks[i] = -1;
							for(j = 0; j < MAX; j++)
							{
								if(connectSocks[j] > 0)
								{
									// nbytes = send(connectSocks[j], &sendFlieListPacket, sizeof(Packet), 0);
									nbytes = SSL_write(ssl[j], &sendFlieListPacket, sizeof(Packet));
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

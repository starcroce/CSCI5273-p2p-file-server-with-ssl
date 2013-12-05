#include <sys/socket.h>
#include <sys/stat.h>
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
#define MAX 30

typedef struct{
    char fileName[128];
    int fileSize;
    char fileOwner[8];
    char ownerIP[128];
    int ownerPort;
} FileInfo;

typedef struct
{
    char names[MAX][8];
    char num;
} NameList;

typedef struct{
    int num;
    char owner[8];
    FileInfo files[MAX];
} FileList;

// type = 1 for file list, type = 0 for command
typedef struct {
    int type;
    char cmd[256];
    FileList fileList;
} Packet;

typedef struct {
    int size;
    char payload[MAXBUFFSIZE];
    char cmd[256];
} DataPacket;

// client get local files and construct local file list
void getFileList(FileList *fileList){
    struct dirent **namelist;
    int n;
    n = scandir(".", &namelist, 0, alphasort);
    if(n < 0)
    {
        perror("scan dir");
        exit(1);
    }
    fileList->num = 0;
    while(n--)
    {
        if((strcmp(namelist[n]->d_name, "..") != 0) && (strcmp(namelist[n]->d_name, ".") != 0 ))
        {
            strcpy(fileList->files[fileList->num].fileName, namelist[n]->d_name);
            fileList->num++;
        }
        free(namelist[n]);
    }
    free(namelist);
}

void printFileList(FileList *fileList){
    int i;
    for(i = 0; i < fileList->num; i++)
    {
        printf("%s||%d||%s||%s||%d\n",
               fileList->files[i].fileName,
               fileList->files[i].fileSize,
               fileList->files[i].fileOwner,
               fileList->files[i].ownerIP,
               fileList->files[i].ownerPort);
    }
}

void copyFileList(FileList *copyto, FileList *from)
{
    int i;
    copyto->num = from->num;
    for (i = 0; i < from->num; i++)
    {
        strcpy(copyto->files[i].fileName, from->files[i].fileName);
        strcpy(copyto->files[i].fileOwner, from->files[i].fileOwner);
        copyto->files[i].fileSize = from->files[i].fileSize;
        strcpy(copyto->files[i].ownerIP, from->files[i].ownerIP);
        copyto->files[i].ownerPort = from->files[i].ownerPort;
    }
}

// merge origin master file list with new file list
void mergeFileList(FileList *master, FileList *newList){
    int i;
    int size = master->num;
    for(i = size; i < size + newList->num; i++)
    {
        strcpy(master->files[i].fileName, newList->files[i - size].fileName);
        strcpy(master->files[i].fileOwner, newList->files[i - size].fileOwner);
        master->files[i].fileSize = newList->files[i - size].fileSize;
        strcpy(master->files[i].ownerIP, newList->files[i - size].ownerIP);
        master->files[i].ownerPort = newList->files[i - size].ownerPort;
    }
    master->num = size + newList->num;
}

// check if client already existed
// 0 for new client, 1 for already existed
int isExisted(NameList *clients, char *name)
{
    int flag = 0;
    int i;
    for(i = 0; i < MAX; i++)
    {
        if(strcmp(clients->names[i], name) == 0)
        {
            flag = 1;
            return flag;
        }
    }
    strcpy(clients->names[clients->num], name);
    clients->num++;
    return flag;
}

// use select to check if there is keyboard input
// 0 for no input, non-zero for input
int kbhit()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

// remove exit clinet's file list from master file list
void deregisterClient(NameList *clients, FileList *master, char *name)
{   
    // remove client name from name list
    int i = 0;
    while(strcmp(clients->names[i], name) != 0)
    {
        i++;
    }
    int j;
    for(j = i; j < clients->num; j++)
    {
        strcpy(clients->names[j], clients->names[j+1]);
    }
    clients->num--;

    // remove client's file list from master file list
    // get the position of exit client in the master file list
    int pos1 = 0, pos2 = 0;
    for(i = 0; i < master->num; i++)
    {
        if(strcmp(master->files[i].fileOwner, name) == 0)
        {
            pos1 = i;
            pos2 = pos1;
            while(strcmp(master->files[pos2].fileOwner, name) == 0)
            {
                pos2++;
            }
            break;
        }
    }
    // overwrite these entries with the following entries
    int range = pos2 - pos1;
    master->num -= range;
    for(i = pos1; i < master->num; i++)
    {
        if(strcmp(master->files[i].fileOwner, name) == 0)
        {
            strcpy(master->files[i].fileName, master->files[i + range].fileName);
            strcpy(master->files[i].fileOwner, master->files[i + range].fileOwner);
            master->files[i].fileSize = master->files[i + range].fileSize;
            strcpy(master->files[i].ownerIP, master->files[i + range].ownerIP);
            master->files[i].ownerPort = master->files[i + range].ownerPort;
        }
    }
}

/*
 * int handleRemotePeerConnection(int connectSock)
 * Handle get file command from remote client
 * return 0 for error, return 1 for success
 */
int handleRemotePeerConnection(int connectSock)
{
    int rtn = 1;
    int nbytes, fsize, size_per_send, i;
    char fname[128];
    DataPacket recvPacket, sendPacket;
    FILE *file;

    // try to receive from remote peer
    nbytes = recv(connectSock, &recvPacket, sizeof(DataPacket), 0);
    if (nbytes > 0)
    {
        // check cmd field
        if (strstr(recvPacket.cmd, "get"))
        {
            // parse file name
            char* sec_arg = strstr(recvPacket.cmd, " ");
            strcpy(fname, sec_arg+1);
            file = fopen(fname, "rb");
            if (file == NULL)
            {
                printf("Failed open file %s.\n", fname);
                strcpy(sendPacket.cmd, "File Not Found");
                nbytes = send(connectSock, &sendPacket, sizeof(DataPacket), 0);
                if (nbytes < 0)
                {
                    perror("error send cmd File Not Found to remote peer");
                }
                close(connectSock);
                rtn = 0;
                return rtn;
            }
            strcpy(sendPacket.cmd, "Sending");
            nbytes = send(connectSock, &sendPacket, sizeof(DataPacket), 0);
            if (nbytes < 0)
            {
                perror("error send cmd Sending to remote peer");
            }
            struct stat st;
            stat(fname, &st);
            fsize = st.st_size;
            int repeats = (int) (fsize/MAXBUFFSIZE) + 1;
            for (i=0; i<repeats; i++)
            {
                size_per_send = (MAXBUFFSIZE) < (fsize-i*MAXBUFFSIZE) ? (MAXBUFFSIZE):(fsize-i*MAXBUFFSIZE);
                int readed = fread(sendPacket.payload, sizeof(char), size_per_send, file);
                sendPacket.size = size_per_send;
                nbytes = send(connectSock, &sendPacket, sizeof(DataPacket), 0);
                if (nbytes < 0)
                {
                    perror("error send file to remote peer");
                }
            }
            fclose(file);
            // receive response from reomte peer
            nbytes = recv(connectSock, &recvPacket, sizeof(DataPacket), 0);
            if (nbytes > 0)
            {
                if (strcmp(recvPacket.cmd, "File received")==0)
                {
                    printf("Remote peer received file %s\n", fname);
                }
            }
            close(connectSock);

        }
        else
        {
            printf("unexpected remote command: %s\n", recvPacket.cmd);
            rtn = 0;
        }

    }
    else
    {
        perror("error recv in handleRemotePeerConnection");
        rtn = 0;
    }
    return rtn;
}

/* int connectRemotePeer(char* cmd, FileList *master)
 * Connect remote client
 * return 0 for error, return 1 for success
 */
int connectRemotePeer(char* cmd, FileList *master)
{
    int rtn = 1;
    DataPacket recvPacket, sendPacket;
    int nbytes, fsize, remoteport, i, connectSock;
    char fname[128], remoteIP[128];
    FILE *file;
    struct sockaddr_in remotePeerAddr;

    // parse file name
    char *sec_arg = strstr(cmd, " ");
    strcpy(fname, sec_arg+1);
    // identify remote peer owns the file
    int index = -1;
    for (i = 0; i < master->num; i++)
    {
        if (strcmp(fname, master->files[i].fileName)==0)
        {
            index = i;
            break;
        }
    }
    if (index < 0)
    {
        printf("File %s is not in master file list, giving up...\n",fname);
        rtn = 0;
        return rtn;
    }
    printf("find file in master list\n");
    fsize = master->files[index].fileSize;
    remoteport = master->files[index].ownerPort;
    strcpy(remoteIP, master->files[index].ownerIP);
    // set remot address
    bzero(&remotePeerAddr, sizeof(remotePeerAddr));
    remotePeerAddr.sin_family = AF_INET;
    remotePeerAddr.sin_addr.s_addr = inet_addr(remoteIP);
    remotePeerAddr.sin_port = htons(remoteport);

    // create new socket and connect remote client
    connectSock = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(connectSock, (struct sockaddr*)&(remotePeerAddr), sizeof(remotePeerAddr)) == 0)
    {
        printf("connected to remote peer\n");
        //sleep(2);
        // once connected, set socket as non block
        /*
        if (fcntl(connectSock, F_SETFL, O_NDELAY)<0)
        {
            perror("error set non-block for peer-peer socket\n");
            rtn = 0;
            return rtn;
        }
        */
        // send command
        strcpy(sendPacket.cmd, cmd);
        nbytes = send(connectSock, &sendPacket, sizeof(DataPacket), 0);
        if (nbytes < 0)
        {
            printf("error send %s cmd to peer", cmd);
            rtn = 0;
            return rtn;
        }
        // recv
        nbytes = recv(connectSock, &recvPacket, sizeof(DataPacket), 0);
        if (nbytes < 0)
        {
            perror("error recv from remote client");
            rtn = 0;
            return rtn;
        }
        if (strstr(recvPacket.cmd, "File Not Found"))
        {
            printf("remote client says: %s\n", recvPacket.cmd);
            rtn = 0;
            return rtn;
        }
        if (strstr(recvPacket.cmd, "Sending"))
        {
            printf("Receiving file %s...\n", fname);
            // receive and write file
            file=fopen(fname,"wb");
            int repeats = (int) (fsize/MAXBUFFSIZE)+1;
            for (i = 0; i < repeats; i++)
            {
                nbytes = recv(connectSock, &recvPacket, sizeof(DataPacket), 0);
                if (nbytes > 0)
                {
                    fwrite(recvPacket.payload, sizeof(char), recvPacket.size, file);
                }
            }
            fclose(file);
            printf("File %s received.\n", fname);
            strcpy(sendPacket.cmd, "File received");
            nbytes = send(connectSock, &sendPacket, sizeof(DataPacket), 0);
            if (nbytes < 0)
            {
                perror("error to send cmd File received");
            }
        }
        else
        {
            printf("Cannot get file %s, remote peer says %s\n",fname, recvPacket.cmd);
            rtn = 0;
        }
        sleep(1);
        close(connectSock);
    }
    else
    {
        printf("failed connect to peer IP %s port %d\n", remoteIP, remoteport);
        perror("");
        rtn = 0;
    }
    return rtn;
}


C=gcc
CFLAGS= -O1 -Wall
all:
	$(CC) $(CFLAGS) -Iutil.h client_PFS.c -o client_PFS
	$(CC) $(CFLAGS) -Iutil.h server_PFS.c -o server_PFS

client:
	$(CC) $(CFLAGS) -Iutil.h client_PFS.c -o client_PFS

server:
	$(CC) $(CFLAGS) -Iutil.h server_PFS.c -o server_PFS

clean:
	        $(RM) server_PFS client_PFS

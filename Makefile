CC=gcc
CFLAGS=-O3 -pthread

all:
	$(CC) $(CFLAGS) client.c -o client.out
	$(CC) $(CFLAGS) serv1.c -o serv1.out
	$(CC) $(CFLAGS) serv2.c -o serv2.out
	$(CC) $(CFLAGS) serv3.c -o serv3.out
	$(CC) $(CFLAGS) serv4.c -o serv4.out
client:
	$(CC) $(CFLAGS) client.c -o client.out

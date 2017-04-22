CC=gcc
CFLAGS=-O3 -pthread

all:
	$(CC) $(CFLAGS) client.c -o client
	$(CC) $(CFLAGS) serv1.c -o serv1
	$(CC) $(CFLAGS) serv2.c -o serv2
	$(CC) $(CFLAGS) serv3.c -o serv3
	$(CC) $(CFLAGS) serv4.c -o serv4
client:
	$(CC) $(CFLAGS) client.c -o client
serv1:
	$(CC) $(CFLAGS) serv1.c -o serv1
serv2:
	$(CC) $(CFLAGS) serv2.c -o serv2
serv3:
	$(CC) $(CFLAGS) serv3.c -o serv3
serv4:
	$(CC) $(CFLAGS) serv4.c -o serv4
clean:
	rm client
	rm serv1
	rm serv2
	rm serv3
	rm serv4

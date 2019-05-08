#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void usage() {
	printf("\nUSAGE:\n");
}

void add_get(int sockfd, char* key) {
	int written;
	int datalen;
	char* data = (char *)malloc(200 * sizeof(data));

	printf("Adding command get with key: '%s'\n", key);

	datalen = sprintf(data, "%c%s~", 103, key);

	data[datalen - 1] = '\0';

	printf("Writing: %d bytes at socket!\n", datalen);

	written = write(sockfd, data, datalen);
	if (written != datalen) {
		printf("Could not write to socket\n");
	}
}

void add_put(int sockfd, const char* key, const char* value) {
	int written;
	int datalen;
	char* data = (char *)malloc(200 * sizeof(data));

	printf("Adding command put with key: '%s' and value: '%s'\n", key, value);

	datalen = sprintf(data, "%c%s~%s~", 112, key, value);

	data[strlen(key) + 1] = '\0';
	data[datalen - 1] = '\0';

	printf("Writing: %d bytes at socket!\n", datalen);

	written = write(sockfd, data, datalen);
	if (written != datalen) {
		printf("Could not write to socket\n");
	}
}

int main(int argc, char** argv){

	if (argc < 4) {
		usage();
		return 0;
	}

	int sockfd;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char* hostname = argv[1];
	short port = atoi(argv[2]);


	printf("Connecting to: %s on port %d\n", hostname, port);
	


	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server = gethostbyname(hostname);
	if (!server) {
		printf("Invalid or unreachable server\n");
		return 0;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

	serv_addr.sin_port = htons(port);

	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		printf("error connecting.\n");
		return 0;
	}


	char command;
	char* key;
	char* value;

	int arg_index = 3;
	char* arg;

	while (arg_index < argc) {
		arg = argv[arg_index];
		if (strcmp(arg, "get") == 0) {
			command = 103;
		}
		else if (strcmp(arg, "put") == 0) {
			command = 112;
		}
		else {
			printf("Wrong command!\n");
			usage();
			return 0;
		}

		++arg_index;
		if (arg_index >= argc) {
			printf("No key given!\n");
			usage();
			return 0;
		}

		key = argv[arg_index];
		++arg_index;


		switch(command) {
			case 103:
				add_get(sockfd, key);
				break;
			case 112:
				if (arg_index >= argc) {
					printf("No value given!\n");
					usage();
					return 0;
				}
				value = argv[arg_index++];
				add_put(sockfd, key, value);
				break;
		}
	}
	close(sockfd);
	return 0;
}

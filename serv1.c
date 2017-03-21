#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>


#define MAX_KEY_LEN 255
#define MAX_VAL_LEN 255


int parse_data(char* data, char* reply);

int main(int argc, char** argv) {
	char* answer = (char*)malloc(500*sizeof(char));

	int sockfd, newsockfd, portno;
	socklen_t clilen;
	char buffer[256];
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	
	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));

	portno = atoi(argv[1]);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}

	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	while (1) {

		if (newsockfd < 0) {
			perror("ERROR on accept");
			exit(1);
		}

		bzero(buffer,256);
		n = read(newsockfd,buffer,255);
		if (n < 0) {
			perror("ERROR reading from socket");
			exit(1);
		}

		if (parse_data(buffer, answer)) {
			write(newsockfd, answer, 20);
		}
	}
	close(newsockfd);
	close(sockfd);
}


void add_key(char* key, char* value) {
	printf("Put KEY: '%s' VALUE: '%s'\n", key, value);
}


int get_key(char* key, char* value) {
	printf("Get KEY: '%s'\n", key);
	memcpy(value, "abc", 4);
	return 0;
}

int parse_data(char* data, char* reply) {
	char mode = data[0];
	char* value;
	char* key = (char *)malloc(MAX_KEY_LEN * sizeof(char));
	int key_len = strlen(data)-1;

	memcpy(key, &data[1], key_len+1);

	switch(mode) {
		case 'g': 
			get_key(key, reply);
			return 1;
		case 'p':
			value = (char *)malloc(MAX_VAL_LEN * sizeof(char));
			memcpy(value, &data[key_len+2], strlen(&data[key_len+2])+1);
			add_key(key, value);
			return 0;
	}
	return 0;
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

//#define NDEBUG
#include <assert.h>

#define MAX_KEY_LEN 255
#define MAX_VAL_LEN 255


// Although not optimal, a simple list should be enough for the purpose of this project
typedef struct L_NODE {
	char* key;
	char* value;

	struct L_NODE* next;
	struct L_NODE* prev;
} L_NODE;

typedef struct LIST {
	L_NODE* first;
	L_NODE* last;
} LIST;

L_NODE* init_node(char* in_key, char* in_value) {

	L_NODE* node = (L_NODE*)malloc(sizeof(L_NODE));

	node->key = (char *)malloc(MAX_KEY_LEN * sizeof(char));
	node->value = (char *)malloc(MAX_VAL_LEN * sizeof(char));
	
	memcpy(node->key, in_key, strlen(in_key)+1);
	memcpy(node->value, in_value, strlen(in_value)+1);

	node->next = NULL;
	node->prev = NULL;

	return node;
}

void init_list(LIST* list) {
	L_NODE* nfirst = (L_NODE*)malloc(sizeof(L_NODE));
	L_NODE* nlast = (L_NODE*)malloc(sizeof(L_NODE));

	nfirst->next = nlast;
	nlast->prev = nfirst;

	list->first = nfirst;
	list->last = nlast;
}

// TODO: Change data structure for binary search?
// Non Critical. Doesn't alter the data structure
L_NODE* find_node(LIST* list, char* search, char* out_found) {
	int cmp_result;
	L_NODE* current = list->first->next;
	while (current != list->last) {
		cmp_result = strcmp(current->key, search);
		if (cmp_result < 0) { // means search > key
			current = current->next;
		}
		else if (cmp_result == 0) {
			*out_found = 1;
			return current;
		}
		else {
			*out_found = 0;
			return current;
		}
	}
	return list->last;
}

// Critical
void insert_node(LIST* list, char* in_key, char* in_value) {
	char found = 0;
	L_NODE* result;
	
	result = find_node(list, in_key, &found);
	if (result == NULL) {
		// Reached end ?? wtf
		assert(0);
	}

	fprintf(stderr, "Was found?: %d\n", found);
	if (found == 1) {
		// Key already exists. 	
		memcpy(result->value, in_value, strlen(in_value)+1);
		return;
	}

	// Insert value before found
	L_NODE* prev_node = result->prev;
	L_NODE* insert = init_node(in_key, in_value);

	insert->prev = prev_node;
	insert->next = result;
	result->prev->next = insert;
	result->prev = insert;
}
/*
int find_value(LIST* list, char* in_key, char* out_value) {
	char found = 0;
	L_NODE* result;

	result = find_node(list, in_key, &found);
	if (found == 0) {
		return 0;
	}
	memcpy(out_value, result->value, strlen(result->value)+1);
	return 1;
}
*/
LIST data;
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

	init_list(&data);

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
	insert_node(&data, key, value);
}


int get_key(char* key, char* value) {
	printf("Get KEY: '%s'\n", key);
	char found = 0;
	L_NODE* result = find_node(&data, key, &found);
	if (found == 0) {
		printf("Key doesn't exist.\n");
		return 0;
	}
	memcpy(value, result->value, strlen(result->value));
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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

//#define NDEBUG
#include <assert.h>

#define BUF_LEN 255
#define MAX_DATA_LEN 255


#define ARRAY_MAX_ELEMENTS 10000

#define SHM_SIZE ARRAY_MAX_ELEMENTS*MAX_DATA_LEN*2


#define SEM_KEY 1511
#define SHM_KEY 1277

pid_t parentPid;
pid_t currentPid;

char Keys[ARRAY_MAX_ELEMENTS][MAX_DATA_LEN] = {};
char Values[ARRAY_MAX_ELEMENTS][MAX_DATA_LEN] = {};

//char shmdata[SHM_SIZE];
char* shmdata;

char* readshm_array(char isValue, int element_index) {
	return (shmdata + (element_index * MAX_DATA_LEN)) + (ARRAY_MAX_ELEMENTS * isValue);	
}

int find_nofill(char* key, char* is_empty) {
	int i;
	for (i=0; i<ARRAY_MAX_ELEMENTS; ++i) {
		if (readshm_array(0, i)[0] == '\0') {
			*is_empty = 1;
			return i;
		}
		if (strcmp(key, readshm_array(0, i)) == 0) {
			*is_empty = 0;
			return i;
		}
	}
	fprintf(stderr, "\n\nArray is too small (%d).\n\n", i);
	assert(1);
	return -1;
}

void insert_pair(char* key, char* value) {
	char is_empty = 0;
	int index = find_nofill(key, &is_empty);
	if (is_empty) {
		memcpy(readshm_array(0, index), key, strlen(key) + 1);
	}
	memcpy(readshm_array(1, index), value, strlen(value) + 1);
}

int find_pair(char* key, char* value) {
	char found_empty = 0;
	int index;
	index = find_nofill(key, &found_empty);

	if (found_empty) {
		// element does not exist
		return 0;
	}

	memcpy(value, readshm_array(1, index), strlen(readshm_array(1, index)) + 1);
	return 1;
}



int parse_data(char* data, char* reply, int* processed, int semid);

int main(int argc, char** argv) {

	char* answer = (char*)malloc(500*sizeof(char));

	int sockfd, newsockfd, portno;
	socklen_t clilen;
	char buffer[BUF_LEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	
	// sem & shm
	int semid;
	int mutex = 0;

	int shmid;
	int num_forks;

	if (argc < 3) {
		fprintf(stderr,"No port or process num provided\n");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));

	portno = atoi(argv[1]);
	num_forks = atoi(argv[2]);
	if (num_forks < 1) {
		fprintf(stderr, "Please atleast 1 process\n");
		exit(1);
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}

	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	
	parentPid = getpid();
//	int iMode = 0;
//	ioctl(sockfd, FIONBIO, &iMode);

	//Create semaphore
	semid = semget(SEM_KEY, 3, 0600 | IPC_CREAT);
	if (semid == -1) {
		perror("Error on semget");
		exit(1);
	}

	if (semctl(semid, mutex, SETVAL, 1) == -1) {
		perror("Error on semctl");
		exit(1);
	}

	shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
	if (shmid < 0) {
		perror("Error on shmget");
		exit(1);
	}

	shmdata = (char*) shmat(shmid, NULL, 0);
	if (shmdata == -1) {
		perror("Error on shmat");
		exit(1);
	}
	bzero(shmdata, SHM_SIZE);


	int i;
	for (i=0; i<num_forks-1; ++i) {
		currentPid = fork();
		if (currentPid > 0) {
			break;
		}
	}



	while(1) {
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
			perror("ERROR on accept");
			exit(1);
		}


		while(1) {
			bzero(buffer, BUF_LEN);
			bzero(answer, BUF_LEN);
			fflush(stdout);
			sleep(1); // debug desync
			n = read(newsockfd,buffer,BUF_LEN);
			if (n < 0) {
				perror("ERROR reading from socket");
				exit(1);
			}
			if (n == 0) {
				break;
			}

//				for (i=0; i<BUF_LEN; ++i) {
//					fprintf(stderr, "%d, ", buffer[i]);
//				}
			int ptr = 0;
			//fprintf(stderr,"Buff len %d, ptr: %d\n", buffer_len(&buffer[ptr]), ptr);
			while (ptr < n - 1) {
				if (parse_data(buffer + ptr, answer, &ptr, semid)) {
					write(newsockfd, answer, BUF_LEN);
				}
			}
		}
		close(newsockfd);
		fprintf(stderr, "Socket closed!\n");
		//exit(0);
	}
	if (semctl(semid, 0, IPC_RMID) == -1) {
		printf("Error on semctl clean");
		exit(1);
	}

	shmdt(shmdata);	
	shmctl(shmid, IPC_RMID, NULL);
	
	close(sockfd);
}

void add_key(char* key, char* value) {
	printf("[%d] PUT %s - %s >>\n", getpid(), key, value);
	insert_pair(key, value);
}

int get_key(char* key, char* value) {
	printf("[%d] GET %s ", getpid(), key);
	char found = find_pair(key, value);

	if (found == 0) {
		printf("- NOT FOUND <<\n");
		return 0;
	}
	printf("- %s <<\n", value);
	//memcpy(value, result->value, strlen(result->value));

	return 1;
}

int parse_data(char* data, char* reply, int* processed, int semid) {
	char mode = data[0];
	char* value = (char *)malloc(MAX_DATA_LEN * sizeof(char));
	char* key = (char *)malloc(MAX_DATA_LEN * sizeof(char));
	int key_len = strlen(data)-1;
	int data_len;
	struct sembuf semaph;

	memcpy(key, &data[1], key_len+1);
	*processed += key_len + 1;
	switch(mode) {
		case 'g': 
			bzero(reply, MAX_DATA_LEN);
			bzero(value, MAX_DATA_LEN);
			if (get_key(key, value) == 0) {
				sprintf(reply, "%c", 110);
			}
			else {
				sprintf(reply, "%c%s", 102, value);
			}
			free(value);
			free(key);
			return 1;
		case 'p':

			
			data_len = strlen(&data[key_len+2])+1;
			memcpy(value, &data[key_len+2], data_len);
			*processed += data_len + 1;

			fprintf(stderr, "@@@@@@@ %d # waiting" , getpid());
			fflush(stderr);
			// decrement sem
			semaph.sem_num = 0;
			semaph.sem_op = -1;
			semaph.sem_flg = 0;
			if (semop(semid, &semaph, 1) == -1) {
				perror("Error on decrement semop");
				exit(1);
			}
			fprintf(stderr, " @@ %d - entering critical\n" , getpid());
			sleep(1);
			add_key(key, value);

			// increment sem
			semaph.sem_num = 0;
			semaph.sem_op = 1;
			semaph.sem_flg = 0;
			fprintf(stderr, "@@@@@@@ %d # unlock\n" , getpid());
			if (semop(semid, &semaph, 1) == -1) {
				perror("Error on increment semop");
				exit(1);
			}

			free(value);
			free(key);
			return 0;
	}
	return -1;
}



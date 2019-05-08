#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <asm/errno.h>
#include <errno.h>

#define BUF_LEN 10000
#define MAX_ENTRIES 1000
#define CHAR_LEN 1000

#define DEBUG_SOCKET 5555

#define NDEBUG

#ifndef NDEBUG
#define debug(__VA_ARGS__) fprintf(stderr, "%s\n", __VA_ARGS__)
#else
#define debug(__VA_ARGS__) do{}while(0)
#endif

void exit_error(char *message) {
    perror(message);
    exit(1);
}

ssize_t readn(int fd, void *void_ptr, size_t n) {
    size_t bytes_left;
    ssize_t bytes_read;
    char *char_ptr;

    char_ptr = void_ptr;
    bytes_left = n;
    while (bytes_left > 0) {
        if ((bytes_read = read(fd, char_ptr, bytes_left)) < 0) {
            if (errno == EINTR) {
                bytes_read = 0;
            } else {
                return -1;
            }
        } else if (bytes_read == 0) {
            break;
        }

        bytes_left -= bytes_read;
        char_ptr += bytes_read;
    }

    return (n - bytes_left);
}

ssize_t writen(int fd, void *void_ptr, size_t n) {
    size_t bytes_left;
    ssize_t bytes_written;
    char *char_ptr;

    char_ptr = void_ptr;
    bytes_left = n;
    while (bytes_left > 0) {
        if ((bytes_written = write(fd, char_ptr, bytes_left)) <= 0) {
            if (bytes_written < 0 && errno == EINTR) {
                bytes_written = 0;
            } else {
                return -1;
            }
        }

        bytes_left -= bytes_written;
        char_ptr += bytes_written;
    }
    return n;
}

void add(char *key, char *value);

char *get(char *key);

typedef struct command {
    char type;  // 'g' for get
    // 'p' for put
    // 'i' for INVALID
    char *key;
    char *value;
};

struct command *parse_command(char *buffer, int *current, int buffer_len) {
    struct command *cmd = (struct command *) malloc(sizeof(struct command));

    if (buffer[*current] != 'g' && buffer[*current] != 'p') {
        cmd->type = 'i';
        return cmd;
    } else {
        cmd->type = buffer[*current];
    }

    (*current)++;

    if (buffer[*current] == '\0') {
        cmd->type = 'i';
        return cmd;
    }
    cmd->key = &buffer[*current];

    while (buffer[*current] != '\0') {
        (*current)++;
        if (*current >= buffer_len) {
            cmd->type = 'i';
            return cmd;
        }
    }
    (*current)++;

    if (cmd->type == 'g') {
        return cmd;
    }

    // cmd->type == 'p'
    if (buffer[*current] == '\0') {
        cmd->type = 'i';
        return cmd;
    }
    cmd->value = &buffer[*current];

    while (buffer[*current] != '\0') {
        (*current)++;
        if (*current >= buffer_len) {
            cmd->type = 'i';
            return cmd;
        }
    }
    (*current)++;
    return cmd;
}

void run_command(struct command *cmd) {
    if (cmd->type == 'p') {
        add(cmd->key, cmd->value);
        return;
    }
    cmd->value = get(cmd->key);
}

void answer_command(struct command *cmd, char *answer_buffer, int *answer_length) {
    if (cmd->type == 'p') {
        return; // nothing to answer
    }

    // cmd->type == 'g'
    if (cmd->value == NULL) {
        // Not found
        answer_buffer[(*answer_length)++] = 110;
    } else {
        size_t value_length = strlen(cmd->value) + 1;
        answer_buffer[(*answer_length)++] = 102;
        memcpy(&answer_buffer[*answer_length], cmd->value, value_length);
        (*answer_length) += value_length;
    }
}

void write_answer(int sockfd, char *answer_buffer, int answer_length) {
    if (writen(sockfd, answer_buffer, answer_length) != answer_length) {
        exit_error("Could not write all the data");
    }
}

int handle_connection(int sockfd) {
    char buffer[BUF_LEN];
    char answer_buffer[BUF_LEN];

    int bytes_read = readn(sockfd, buffer, BUF_LEN);
    int answer_length = 0;

    if (bytes_read < 0) {
        exit_error("Could not read from socket");
    } else if (bytes_read == 0) {
        // No more data to read
    }

    int buffer_index = 0;
    struct command *cmd;

    do {
        cmd = parse_command(buffer, &buffer_index, bytes_read);

        if (cmd->type == 'i') { // Invalid command.
            write_answer(sockfd, answer_buffer, answer_length);
            close(sockfd);
            free(cmd);
            debug("Invalid input. Dropping client.");
            return 1;
        }

        run_command(cmd);
        answer_command(cmd, answer_buffer, &answer_length);

        free(cmd);
    } while (buffer_index < bytes_read);

    write_answer(sockfd, answer_buffer, answer_length);
    close(sockfd);
    return 1;
}


int main(int argc, char **argv) {
    int sockfd;
    int newsockfd;
    socklen_t cli_len;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

#ifndef NDEBUG
    printf("Automatically using port %d in debug mode\n", DEBUG_SOCKET);
#else
    if (argc < 2) {
        printf("Please provide port number as an argument.\n");
        return 0;
    }
#endif

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        exit_error("Socket error.");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
#ifndef NDEBUG
    serv_addr.sin_port = htons(DEBUG_SOCKET);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        serv_addr.sin_port = htons(DEBUG_SOCKET+1);
        printf("Automatically using port %d in debug mode\n", DEBUG_SOCKET + 1);
        bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    }
#else
    serv_addr.sin_port = htons(atoi(argv[1]));
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        exit_error("Error binding socket");
    }
#endif


    listen(sockfd, 5);

    cli_len = sizeof(cli_addr);

    do {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);
        if (newsockfd < 0) {
            exit_error("Error accepting incoming connection");
        }
    } while (handle_connection(newsockfd));

    close(sockfd);
}

//
// Data implementation.
//
//   char* get(char *key);
//      Returns pointer to the value
//      Returns NULL if the key is not found
//
//   void add(char *key, char *value);
//

char DatabaseKeys[MAX_ENTRIES][CHAR_LEN];
char DatabaseValues[MAX_ENTRIES][CHAR_LEN];
int CurrentEntries = 0;

int find_key(char *key) {
    int i;
    for (i = 0; i < CurrentEntries; ++i) {
        if (strcmp(DatabaseKeys[i], key) == 0) {
            return i;
        }
    }
    // Key not found
    return -1;
}

void set_key(int index, char *value) {
#ifndef NDEBUG
    if (index > CurrentEntries) {
        debug("set_key: Asked for key out of index");
    }
#endif
    strcpy(DatabaseValues[index], value);
}

void add_key(char *key, char *value) {
    strcpy(DatabaseKeys[CurrentEntries], key);
    strcpy(DatabaseValues[CurrentEntries], value);
    CurrentEntries++;
}

char *get_key(int key_index) {
    return DatabaseValues[key_index];
}


void add(char *key, char *value) {
    int key_index = find_key(key);

    if (key_index != -1) {
        set_key(key_index, value);
    } else {
        add_key(key, value);
    }
}

char *get(char *key) {
    int key_index = find_key(key);

    if (key_index != -1) {
        return get_key(key_index);
    }

    return NULL;
}
#include <stdio.h>

void usage() {
	printf("\nUSAGE:\n");
}

void add_get(const char* key) {
	printf("Adding command get with key: '%s'\n", key);
}

void add_put(const char* key, const char* value) {
	printf("Adding command put with key: '%s' and value: '%s'\n", key, value);
}

int main(int argc, char** argv){

	if (argc < 4) {
		usage();
		return 0;
	}

	char* hostname = argv[1];
	short port = atoi(argv[2]);

	printf("Connecting to: %s on port %d\n", hostname, port);

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
				add_get(key);
				break;
			case 112:
				if (arg_index >= argc) {
					printf("No value given!\n");
					usage();
					return 0;
				}
				value = argv[arg_index++];
				add_put(key, value);
				break;
		}
	}

	return 0;
}

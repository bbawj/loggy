loggy: loggy.c common.c keys.c
	$(CC) thirdparty/cJSON.c loggy.c keys.c common.c -o loggy -Wall -Wextra -pedantic -std=c99

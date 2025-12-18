#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../src/strmap.h"

#define MIN_STR_LEN 5
#define MAX_STR_LEN 50
#define NUM_STR 100000

#define OPS 100000
#define TEST_EVERY 500
#define PRINT_EVERY 1000

#define	MIN_VAL -5000
#define MAX_VAL 5000

// inclusive
int randint(int min, int max) {
	return rand() % (max - min + 1) + min;
}

char *rand_str() {
	int len = randint(MIN_STR_LEN, MAX_STR_LEN);
	char *str = malloc((len + 1) * sizeof(char));
	for (int i = 0; i < len; i++)
		str[i] = randint('a', 'z');
	str[len] = 0;
	return str;
}

int main() {
	srand(0);

	char **strs = malloc(NUM_STR * sizeof(const char *));
	int *values = malloc(NUM_STR * sizeof(int));

	strmap map = strmap_new();

	for (int i = 0 ; i < NUM_STR; i++) {
		char *str;
		do
			str = rand_str();
		while (strmap_get(&map, str) != NULL);

		strs[i] = str;
		values[i] = randint(MIN_VAL, MAX_VAL);
		strmap_set(&map, strs[i], &values[i], sizeof(values[i]));
	}

	for (int op = 1; op <= OPS; op++) {
		if (op % TEST_EVERY == 0) {
			for (int i = 0; i < NUM_STR; i++) {
				const char *str = strs[i];
				int map_value = *(int *) strmap_get(&map, str);
				if (map_value != values[i]) {
					fprintf(stderr, "ERROR! op %d\n", op);
					goto cleanup;
				}
			}
		}
		if (op % PRINT_EVERY == 0) {
			printf("%d/%d ok\n", op, OPS);
			fflush(stdout);
		}

		int index = randint(0, NUM_STR - 1);
		values[index] = randint(MIN_VAL, MAX_VAL);
		strmap_set(&map, strs[index], &values[index], sizeof(values[index]));
	}

	printf("ok\n");
	
cleanup:
	for (int i = 0; i < NUM_STR; i++)
		free(strs[i]);
	free(strs);
	free(values);

	strmap_free(&map);

	return 0;
}

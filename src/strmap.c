#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "strmap.h"

#define REHASH_FACTOR 0.75
#define REHASH_MULTIPLY 2

// djb2 algorithm: http://www.cse.yorku.ca/~oz/hash.html
static uint64_t djb2_hash(const unsigned char *str) {
	uint64_t hash = 5381;
	uint32_t c;

	while ((c = *(str++)))
		hash = ((hash << 5) + hash) + c; // hash * 33 + c

	return hash;
}

static uint64_t occupied_buckets = 0;
static uint64_t bucket_count = 100;

struct strmap_list_node {
	struct strmap_list_node *next;
	const char *str;
	void *value;
	size_t value_size;
};

strmap strmap_new() {
	strmap map = calloc(bucket_count, sizeof(struct strmap_list_node *));
	return map;
}

void strmap_set_internal(strmap *map_ptr, const char *str, void *value, size_t value_size, bool copy_value);

void strmap_rehash(strmap *map_ptr) {
	struct strmap_list_node **old_map = *map_ptr;

	uint64_t old_bucket_count = bucket_count;
	bucket_count *= REHASH_MULTIPLY;

	// printf("[STRMAP] rehash %lu to %lu\n", old_bucket_count, bucket_count);

	strmap new_map = calloc(bucket_count, sizeof(struct strmap_list_node *));
	for (uint64_t i = 0; i < old_bucket_count; i++) {
		struct strmap_list_node *cur = old_map[i];
		while (cur != NULL) {
			// insert, but do not copy the value
			// (since we are rehashing, cur->value is allocated with malloc in strmap_set)
			strmap_set_internal(&new_map, cur->str, cur->value, cur->value_size, false);
			cur = cur->next;
		}
	}

	for (uint64_t i = 0; i < old_bucket_count; i++) {
		struct strmap_list_node *cur = old_map[i], *next = NULL;
		if (cur == NULL)
			continue;
		while (cur != NULL) {
			next = cur->next;
			free(cur);
			cur = next;
		}
	}

	free(*map_ptr);
	*map_ptr = new_map;
}

void strmap_set_internal(strmap *map_ptr, const char *str, void *value, size_t value_size, bool copy_value) {
	struct strmap_list_node **map = *map_ptr;

	uint64_t hash = djb2_hash((const unsigned char *) str);
	struct strmap_list_node **head = &map[hash % bucket_count];

	void *value_alloc;
	if (copy_value) {
		value_alloc = malloc(value_size);
		memcpy(value_alloc, value, value_size);
	}
	else 
		value_alloc = value;


	if (*head == NULL) {
		struct strmap_list_node *new_node = malloc(sizeof(struct strmap_list_node));
		new_node->next = NULL;
		new_node->str = str;
		new_node->value = value_alloc;
		new_node->value_size = value_size;
		*head = new_node;
		occupied_buckets++;

		if (occupied_buckets >= bucket_count * REHASH_FACTOR)
			strmap_rehash(map_ptr);

		return;
	}

	struct strmap_list_node *cur, *next = *head;
	do {
		cur = next;
		if (strcmp(cur->str, str) == 0) {
			free(cur->value);
			cur->value = value_alloc;
			return;
		}
	} while ((next = cur->next) != NULL);

	struct strmap_list_node *new_node = malloc(sizeof(struct strmap_list_node));
	new_node->next = NULL;
	new_node->str = str;
	new_node->value = value_alloc;
	new_node->value_size = value_size;
	cur->next = new_node;
}

void *strmap_get(const strmap *map_ptr, const char *str) {
	struct strmap_list_node **map = *map_ptr;

	uint64_t hash = djb2_hash((const unsigned char *) str);
	struct strmap_list_node *cur = map[hash % bucket_count];

	while (cur != NULL) {
		if (strcmp(cur->str, str) == 0)
			return cur->value;
		cur = cur->next;
	}

	return NULL;
}

// this WILL COPY the value, need to specify the size of the value
// does NOT copy the string
// the "value" pointer can be freed/exit scope
void strmap_set(strmap *map_ptr, const char *str, void *value, size_t value_size) {
	strmap_set_internal(map_ptr, str, value, value_size, true);
}

void strmap_free(const strmap *map_ptr) {
	struct strmap_list_node **map = *map_ptr;
	for (uint64_t i = 0; i < bucket_count; i++) {
		struct strmap_list_node *cur = map[i], *next = NULL;
		if (cur == NULL)
			continue;
		while (cur != NULL) {
			next = cur->next;
			free(cur->value);
			free(cur);
			cur = next;
		}
	}

	free(*map_ptr);
}


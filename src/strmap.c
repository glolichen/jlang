#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "strmap.h"

#define STRMAP_STARTING_BUCKETS 100
#define STRMAP_REHASH_FACTOR 0.75
#define STRMAP_REHASH_MULTIPLY 2

// djb2 algorithm: http://www.cse.yorku.ca/~oz/hash.html
static uint64_t djb2_hash(const unsigned char *str) {
	uint64_t hash = 5381;
	uint32_t c;

	while ((c = *(str++)))
		hash = ((hash << 5) + hash) + c; // hash * 33 + c

	return hash;
}

struct strmap strmap_new() {
	struct strmap map = {
		.list = calloc(STRMAP_STARTING_BUCKETS, sizeof(struct strmap_list_node *)),
		.bucket_count = STRMAP_STARTING_BUCKETS,
		.occupied_buckets = 0
	};
	return map;
}

struct strmap strmap_copy(const struct strmap *old_map_ptr) {
	struct strmap new_map = {
		.list = calloc(old_map_ptr->bucket_count, sizeof(struct strmap_list_node *)),
		.bucket_count = old_map_ptr->bucket_count,
		.occupied_buckets = old_map_ptr->occupied_buckets
	};

	for (uint64_t i = 0; i < old_map_ptr->bucket_count; i++) {
		const struct strmap_list_node *cur_old = old_map_ptr->list[i];
		struct strmap_list_node **cur = &new_map.list[i];
		while (cur_old != NULL) {
			struct strmap_list_node *new_node = malloc(sizeof(struct strmap_list_node));

			new_node->next = NULL;
			new_node->str = cur_old->str;
			new_node->value = malloc(cur_old->value_size);
			new_node->value_size = cur_old->value_size;
			memcpy(new_node->value, cur_old->value, cur_old->value_size);

			*cur = new_node;
			cur = &(*cur)->next;
			cur_old = cur_old->next;
		}
	}

	return new_map;
}

static void strmap_set_internal(struct strmap *map_ptr, const char *str, void *value, size_t value_size, bool copy_value);

static void strmap_rehash(struct strmap *map_ptr) {
	struct strmap_list_node **old_map = map_ptr->list;

	uint64_t old_bucket_count = map_ptr->bucket_count;
	map_ptr->bucket_count *= STRMAP_REHASH_MULTIPLY;

	// printf("[STRMAP] rehash %lu to %lu\n", old_bucket_count, bucket_count);

	map_ptr->list = calloc(map_ptr->bucket_count, sizeof(struct strmap_list_node *));
	for (uint64_t i = 0; i < old_bucket_count; i++) {
		struct strmap_list_node *cur = old_map[i];
		while (cur != NULL) {
			// insert, but do not copy the value
			// (since we are rehashing, cur->value is allocated with malloc in strmap_set)
			strmap_set_internal(map_ptr, cur->str, cur->value, cur->value_size, false);
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

	free(old_map);
}

static void strmap_set_internal(struct strmap *map_ptr, const char *str, void *value, size_t value_size, bool copy_value) {
	struct strmap_list_node **map = map_ptr->list;

	uint64_t hash = djb2_hash((const unsigned char *) str);
	struct strmap_list_node **head = &map[hash % map_ptr->bucket_count];

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
		map_ptr->occupied_buckets++;

		if (map_ptr->occupied_buckets >= map_ptr->bucket_count * STRMAP_REHASH_FACTOR)
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

// this will LITERALLY return the POINTER TO WHAT IS STORED IN THE MAP
// if it is modified, the value in the map will also be modified
void *strmap_get(const struct strmap *map_ptr, const char *str) {
	struct strmap_list_node **map = map_ptr->list;

	uint64_t hash = djb2_hash((const unsigned char *) str);
	struct strmap_list_node *cur = map[hash % map_ptr->bucket_count];

	while (cur != NULL) {
		if (strcmp(cur->str, str) == 0)
			return cur->value;
		cur = cur->next;
	}

	return NULL;
}

// this WILL COPY the value, need to specify the size of the value
// does NOT copy the key (string)
// the "value" pointer can be freed/exit scope
void strmap_set(struct strmap *map_ptr, const char *str, void *value, size_t value_size) {
	strmap_set_internal(map_ptr, str, value, value_size, true);
}

// if ret_value = true, function will return pointer to value
// the returned pointer must be freed at some point
// if ret_value = false, this will always return NULL
void *strmap_remove(struct strmap *map_ptr, const char *str, bool ret_value) {
	struct strmap_list_node **map = map_ptr->list;

	uint64_t hash = djb2_hash((const unsigned char *) str);

	struct strmap_list_node **head = &map[hash % map_ptr->bucket_count];
	if (strcmp((*head)->str, str) == 0) {
		void *value = ret_value ? (*head)->value : NULL;
		struct strmap_list_node *next = (*head)->next;
		if (!ret_value)
			free((*head)->value);
		free(*head);
		*head = next;
		return value;
	}

	struct strmap_list_node *prev = *head;
	struct strmap_list_node *cur = prev->next;

	while (cur != NULL) {
		if (strcmp(cur->str, str) == 0) {
			void *value = ret_value ? cur->value : NULL;
			struct strmap_list_node *next = cur->next;
			if (!ret_value)
				free(cur->value);
			free(cur);
			prev->next = next;
			return value;
		}
		prev = cur;
		cur = cur->next;
	}

	return NULL;
}

// will free all VALUES (these have been copied from original, by strmap_set)
// will not free KEYS (strings)
void strmap_free(const struct strmap *map_ptr) {
	struct strmap_list_node **map = map_ptr->list;
	for (uint64_t i = 0; i < map_ptr->bucket_count; i++) {
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

	free(map_ptr->list);
}


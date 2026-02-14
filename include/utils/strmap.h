#ifndef STRMAP_H
#define STRMAP_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

// highly discouraged to use this struct outside of strmap.c unless necessary!!
// might be necessary: iterating through keys/values
struct strmap_list_node {
	struct strmap_list_node *next;
	const char *str;
	void *value;
	size_t value_size;
};

struct strmap {
	struct strmap_list_node **list;
	uint64_t occupied_buckets, bucket_count;
};

struct strmap strmap_new();
struct strmap strmap_copy(const struct strmap *old_map_ptr);
void strmap_set(struct strmap *map_ptr, const char *str, void *value, size_t value_size);
void *strmap_get(const struct strmap *map_ptr, const char *str);
void *strmap_remove(struct strmap *map_ptr, const char *str, bool ret_value);
void strmap_free(const struct strmap *map_ptr);

#endif

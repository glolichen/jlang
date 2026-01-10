#ifndef VARMAP_H

#include <stdint.h>
#include <stdlib.h>

struct strmap {
	struct strmap_list_node **list;
	uint64_t occupied_buckets, bucket_count;
};

struct strmap strmap_new();
void strmap_set(struct strmap *map_ptr, const char *str, void *value, size_t value_size);
void *strmap_get(const struct strmap *map_ptr, const char *str);
void strmap_free(const struct strmap *map_ptr);

#endif

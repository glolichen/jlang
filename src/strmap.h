#ifndef VARMAP_H

#include <stdint.h>
#include <stdlib.h>

typedef void * strmap;

strmap strmap_new();
void strmap_set(strmap *map_ptr, const char *str, void *value, size_t value_size);
void *strmap_get(const strmap *map_ptr, const char *str);
void strmap_free(const strmap *map_ptr);

#endif

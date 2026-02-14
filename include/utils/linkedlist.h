#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

struct ll_list_node {
	struct ll_list_node *next;
	void *data;
};

struct ll_list_node *ll_new(void);
void ll_add(struct ll_list_node **ll_head, void *data);
void ll_free(struct ll_list_node **ll_head);

#endif


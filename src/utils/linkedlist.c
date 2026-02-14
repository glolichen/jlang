#include "utils/linkedlist.h"

struct ll_list_node *ll_new(void) {
	return NULL;
}

// does not copy the data
void ll_add(struct ll_list_node **ll_head, void *data) {
	struct ll_list_node *new_node = malloc(sizeof(struct ll_list_node));
	new_node->next = NULL;
	new_node->data = data;
	if (*ll_head == NULL) {
		*ll_head = new_node;
		return;
	}

	struct ll_list_node *cur = *ll_head;
	while (cur->next != NULL) {
		cur = cur->next;
	}

	cur->next = new_node;
}
void ll_free(struct ll_list_node **ll_head) {
	struct ll_list_node *cur = *ll_head, *next = cur;
	while (next != NULL) {
		next = cur->next;
		free(cur);
		cur = next;
	}
}


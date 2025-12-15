#include <stdbool.h>
#include <stdio.h>

#include "ast.h"
#include "lex.h"

static const char *ast_node_type_to_str(enum ast_node_type type) {
	switch(type) {
		case AST_ROOT:
			return "ROOT";
		case AST_EXPR_NO_COMP:
			return "EXPR_NO_COMP";
		case AST_EXPR:
			return "EXPR";
		case AST_EXPR_LIST:
			return "EXPR_LIST";
		case AST_TERM:
			return "TERM";
		case AST_FACTOR:
			return "FACTOR";
		case AST_LEAF:
			return "LEAF";
		case AST_STMT:
			return "STMT";
		case AST_STMT_LIST:
			return "STMT_LIST";
		case AST_ASSIGN:
			return "ASSIGN";
		case AST_FUNC_CALL:
			return "FUNC_CALL";
		case AST_CONDITIONAL:
			return "CONDITIONAL";
	}
	return "";
}

struct ast_node ast_new_node(enum ast_node_type type) {
	return (struct ast_node) {
		.value.children = ast_new_node_list(),
		.type = type,
	};
}

// will free all lex_token's under the node/tree
// those tokens will be set to NULL so that they are not freed again
void ast_free_node(struct ast_node *node) {
	if (node->type == AST_LEAF)
		lex_free_token(&node->value.token);
	else
		ast_free_node_list(&node->value.children);
}

struct ast_node_list ast_new_node_list(void) {
	struct ast_node_list list;
	list.l = NULL, list.size = 0, list.capacity = 0;
	return list;
}
void ast_node_list_append(struct ast_node_list *list, struct ast_node node) {
	if (list->capacity == 0) {
		list->capacity = 1, list->size = 1;
		list->l = malloc(1 * sizeof(struct ast_node));
		list->l[0] = node;
		return;
	}
	if (list->size + 1 <= list->capacity) {
		list->l[list->size++] = node;
		return;
	}

	list->capacity *= 2;
	list->l = realloc(list->l, list->capacity * sizeof(struct ast_node));
	list->l[list->size++] = node;
}

void ast_free_node_list(struct ast_node_list *list) {
	for (size_t i = 0; i < list->size; i++)
		ast_free_node(&list->l[i]);
	free(list->l);
	list->capacity = 0, list->size = 0;
}

// TODO: Safety -- if current node type is terminal, do not allow this
// if adding node type is terminal, use insert_leaf instead
size_t ast_insert_node(struct ast_node *node, enum ast_node_type type) {
	if (node->type == AST_LEAF || type == AST_LEAF)
		return -1;

	struct ast_node_list *list = &node->value.children;

	struct ast_node new_node;
	new_node.type = type;
	new_node.value.children = ast_new_node_list();
	ast_node_list_append(list, new_node);

	return list->size - 1;
}

size_t ast_insert_leaf(struct ast_node *node, const struct lex_token *token) {
	if (node->type == AST_LEAF)
		return -1;

	struct ast_node_list *list = &node->value.children;

	struct ast_node new_node;
	new_node.type = AST_LEAF;
	new_node.value.token = *token;
	ast_node_list_append(list, new_node);

	return list->size - 1;
}

bool ast_remove_node(struct ast_node *node, size_t index) {
	if (node->type == AST_LEAF)
		return false;
	if (index >= node->value.children.size)
		return false;

	ast_free_node(&node->value.children.l[index]);
	// move all after index forward
	for (size_t i = index + 1; i < node->value.children.size; i++)
		node->value.children.l[i - 1] = node->value.children.l[i];

	node->value.children.size--;

	return true;
}




struct ast_ll_node_data {
	const struct ast_node *node;
	size_t level, parent_id;
};
struct ast_ll_node {
	struct ast_ll_node *next;
	struct ast_ll_node_data data;
};
struct ast_queue {
	struct ast_ll_node *head, *tail;
};

static bool ast_queue_is_empty(const struct ast_queue *queue) {
	return queue->head == NULL || queue->tail == NULL;
}
static struct ast_queue new_ast_queue(void) {
	struct ast_queue queue;
	queue.head = NULL, queue.tail = NULL;
	return queue;
}
static void ast_queue_append(
	struct ast_queue *queue,
	const struct ast_node *node,
	size_t level, size_t parent_id
) {
	struct ast_ll_node *new_node = malloc(sizeof(struct ast_ll_node));
	new_node->next = NULL;
	new_node->data = (struct ast_ll_node_data) {
		.node = node, .level = level, .parent_id = parent_id
	};

	if (ast_queue_is_empty(queue)) {
		queue->head = new_node, queue->tail = new_node;
		return;
	}

	queue->tail->next = new_node;
	queue->tail = new_node;
}
struct ast_ll_node_data ast_queue_pop(struct ast_queue *queue) {
	if (ast_queue_is_empty(queue))
		return (struct ast_ll_node_data) { .node = NULL, .level = -1, .parent_id = -1 };

	struct ast_ll_node *cur_head = queue->head;
	const struct ast_ll_node_data data = cur_head->data;

	queue->head = queue->head->next;
	if (queue->head == NULL)
		queue->tail = NULL;

	free(cur_head);

	return data;
}

static void ast_print_in_order(const struct ast_node *node) {
	if (node->type == AST_LEAF) {
		lex_print_token(&node->value.token);
		return;
	}
	struct ast_node_list children = node->value.children;
	for (size_t i = 0; i < children.size; i++)
		ast_print_in_order(&children.l[i]);
}

void ast_print(const struct ast_node *root) {
	struct ast_queue queue = new_ast_queue();

	size_t counter = 0, current_level = 0;
	ast_queue_append(&queue, root, 1, -1);
	while (!ast_queue_is_empty(&queue)) {
		struct ast_ll_node_data cur = ast_queue_pop(&queue);
		if (cur.level != current_level) {
			printf("\n========== LEVEL %zu ==========\n\n", cur.level);
			current_level = cur.level;
		}

		if (cur.node->type == AST_LEAF) {
			printf("%zu -> ID %zu, leaf: ", cur.parent_id, counter++);
			lex_print_token(&cur.node->value.token);
			continue;
		}

		if (cur.parent_id != (size_t) -1)
			printf("%zu -> ", cur.parent_id);
		else
			printf("root ");
		printf(
			"ID %zu, nonterminal: %s, %zu children\n",
			counter,
			ast_node_type_to_str(cur.node->type),
			cur.node->value.children.size
		);

		struct ast_node_list children = cur.node->value.children;
		for (size_t i = 0; i < children.size; i++)
			ast_queue_append(&queue, &children.l[i], cur.level + 1, counter);

		counter++;
	}

	printf("\n========== TERMINALS ==========\n\n");
	ast_print_in_order(root);
	printf("\n===============================\n\n");
}

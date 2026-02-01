#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include "lex.h"

enum ast_node_type {
	AST_ROOT,
	AST_EXPR_NO_COMP,
	AST_EXPR,
	AST_EXPR_LIST,
	AST_TERM,
	AST_FACTOR,
	AST_LEAF,
	AST_STMT,
	AST_STMT_LIST,
	AST_ASSIGN,
	AST_FUNC_CALL,
	AST_CONDITIONAL,
	AST_FOR,
	AST_RETURN
};
struct ast_node_list {
	struct ast_node *l;
	size_t size, capacity;
};
union ast_node_value {
	struct ast_node_list children;
	struct lex_token token;
};
struct ast_node {
	enum ast_node_type type;

	// terminals are lex_token, nonterminals are list of ast_node
	// i.e. if type is AST_LEAF, then "value" is lex_token
	// if not, then it is a list of child nodes
	union ast_node_value value;
};

struct ast_node ast_new_node(enum ast_node_type type);
void ast_free_node(struct ast_node *node);

struct ast_node_list ast_new_node_list(void);
void ast_node_list_append(struct ast_node_list *list, struct ast_node token);
void ast_free_node_list(struct ast_node_list *list);

size_t ast_insert_node(struct ast_node *node, enum ast_node_type type);
size_t ast_insert_leaf(struct ast_node *node, const struct lex_token *token);

bool ast_remove_node(struct ast_node *node, size_t index);

void ast_print(const struct ast_node *root);

#endif


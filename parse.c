#include <stdio.h>
#include <stdbool.h>

#include "lex.h"
#include "parse.h"
#include "ast.h"

struct lex_token_list token_list;
size_t current_index = 0;

// move onto the next lexeme
static void next() {
	current_index++;
}
static void prev() {
	current_index--;
}
static void set_token(int index) {
	current_index = index;
}
static struct lex_token *get_cur(void) {
	return &token_list.l[current_index];
}

// check if current lexeme is OK (in the list)
static bool is_type(enum lex_token_type type) {
	return token_list.l[current_index].type == type;
}
// same as accept but will error on failure
static bool expects(const enum lex_token_type *type, size_t amount) {
	enum lex_token_type cur_type = token_list.l[current_index].type;
	for (size_t i = 0; i < amount; i++) {
		if (cur_type == type[i])
			return true;
	}
	fprintf(stderr, "ERROR! (1)\n");
	return false;
}
static bool expect(enum lex_token_type type) {
	if (is_type(type))
		return true;
	fprintf(stderr, "ERROR! (1)\n");
	return false;
}

static void expression(struct ast_node *node);

static void factor(struct ast_node *node) {
	if (is_type(LEX_NUMBER) || is_type(LEX_IDENTIFIER)) {
		ast_insert_leaf(node, get_cur());
		next();
		return;
	}
	if (is_type(LEX_LEFT_PAREN)) {
		ast_insert_leaf(node, get_cur());
		next();

		size_t new_index = ast_insert_node(node, AST_EXPR);
		expression(&node->value.children.l[new_index]);

		expect(LEX_RIGHT_PAREN);
		ast_insert_leaf(node, get_cur());

		next();

		return;
	}
	fprintf(stderr, "ERROR! (2)\n");
}

static void term(struct ast_node *node) {
	size_t new_index = ast_insert_node(node, AST_FACTOR);
	factor(&node->value.children.l[new_index]);

	while (is_type(LEX_STAR) || is_type(LEX_SLASH)) {
		ast_insert_leaf(node, get_cur());
		next();
		new_index = ast_insert_node(node, AST_FACTOR);
		factor(&node->value.children.l[new_index]);
	}
}

static void expression(struct ast_node *node) {
	if (is_type(LEX_PLUS) || is_type(LEX_MINUS)) {
		ast_insert_leaf(node, get_cur());
		next();
	}

	size_t new_index = ast_insert_node(node, AST_TERM);
	term(&node->value.children.l[new_index]);

	while (is_type(LEX_PLUS) || is_type(LEX_MINUS)) {
		ast_insert_leaf(node, get_cur());
		next();

		new_index = ast_insert_node(node, AST_TERM);
		term(&node->value.children.l[new_index]);
	}
}

// static void condition(struct ast_node *node) {
// 	size_t new_index = ast_insert_node(node, AST_EXPR);
// 	expression(&node->value.children.l[new_index]);
//
// 	enum lex_token_type acceptable[] = {
// 		LEX_EQUAL_EQUAL, LEX_BANG_EQUAL,
// 		LEX_LESS, LEX_LESS_EQUAL,
// 		LEX_GREATER, LEX_GREATER_EQUAL
// 	};
//
// 	if (expects(acceptable, sizeof(acceptable) / sizeof(acceptable[0]))) {
// 		ast_insert_leaf(node, get_cur());
// 		next();
//
// 		new_index = ast_insert_node(node, AST_EXPR);
// 		expression(&node->value.children.l[new_index]);
// 	}
// }

static bool assignment(struct ast_node *node) {
	// 1 token lookahead, ensure after identifier is equal character
	if (!is_type(LEX_IDENTIFIER))
		return false;
	next();
	if (!is_type(LEX_EQUAL))
		return false;

	// move back
	prev();
	ast_insert_leaf(node, get_cur());
	next();
	ast_insert_leaf(node, get_cur());
	next();

	size_t new_index = ast_insert_node(node, AST_EXPR);
	expression(&node->value.children.l[new_index]);

	expect(LEX_SEMICOLON);
	next();

	return true;
}

static void goal(struct ast_node *node) {
	size_t new_index = ast_insert_node(node, AST_ASSIGN);
	bool ok = assignment(&node->value.children.l[new_index]);

	printf("success? %u\n", ok);
	// expression(node);
}

void parse(const struct lex_token_list *tokens, struct ast_node *root) {
	token_list = *tokens;
	current_index = 0;
	goal(root);
}


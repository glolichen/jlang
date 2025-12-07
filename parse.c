#include <stdio.h>
#include <stdbool.h>

#include "lex.h"
#include "parse.h"
#include "ast.h"

const enum lex_token_type COMP_OPS[] = {
	LEX_EQUAL_EQUAL, LEX_BANG_EQUAL,
	LEX_LESS, LEX_LESS_EQUAL,
	LEX_GREATER, LEX_GREATER_EQUAL
};

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
static const struct lex_token *get_cur(void) {
	return &token_list.l[current_index];
}

// check if current lexeme is OK (in the list)
static bool is_type(enum lex_token_type type) {
	return token_list.l[current_index].type == type;
}
static bool is_types(const enum lex_token_type *type, size_t amount) {
	enum lex_token_type cur_type = token_list.l[current_index].type;
	for (size_t i = 0; i < amount; i++) {
		if (cur_type == type[i])
			return true;
	}
	return false;
}


// same as accept but will error on failure
// static bool expects(const enum lex_token_type *type, size_t amount) {
// 	enum lex_token_type cur_type = token_list.l[current_index].type;
// 	for (size_t i = 0; i < amount; i++) {
// 		if (cur_type == type[i])
// 			return true;
// 	}
// 	fprintf(stderr, "ERROR! (1)\n");
// 	return false;
// }
static bool expect(enum lex_token_type type) {
	if (is_type(type))
		return true;
	fprintf(stderr, "ERROR! (1)\n");
	return false;
}

static void expr_no_comp(struct ast_node *node);
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

static void expr_no_comp(struct ast_node *node) {
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

static void expression(struct ast_node *node) {
	size_t new_index = ast_insert_node(node, AST_EXPR_NO_COMP);
	expr_no_comp(&node->value.children.l[new_index]);

	if (!is_types(COMP_OPS, sizeof(COMP_OPS) / sizeof(COMP_OPS[0])))
		return;
	
	ast_insert_leaf(node, get_cur());
	next();

	new_index = ast_insert_node(node, AST_EXPR_NO_COMP);
	expr_no_comp(&node->value.children.l[new_index]);
}

static bool expression_list(struct ast_node *node) {
	if (!is_type(LEX_LEFT_PAREN))
		return false;

	next();
	if (is_type(LEX_RIGHT_PAREN)) {
		next();
		return true;
	}

	size_t new_index;
	while (true) {
		new_index = ast_insert_node(node, AST_EXPR);
		expression(&node->value.children.l[new_index]);

		if (is_type(LEX_RIGHT_PAREN)) {
			next();
			return true;
		}

		if (!expect(LEX_COMMA))
			break;
		next();
	}

	return false;
}


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

static bool func_call(struct ast_node *node) {
	if (!is_type(LEX_IDENTIFIER))
		return false;

	ast_insert_leaf(node, get_cur());
	next();

	size_t new_index = ast_insert_node(node, AST_EXPR_LIST);
	if (!expression_list(&node->value.children.l[new_index])) {
		fprintf(stderr, "ERROR! (3)\n");
		return false;
	}

	expect(LEX_SEMICOLON);
	next();

	return true;
}

static bool conditional(struct ast_node *node);

static bool statement(struct ast_node *node) {
	if (is_type(LEX_SEMICOLON)) {
		next();
		return true;
	}

	size_t start_index = current_index;

	size_t new_index = ast_insert_node(node, AST_ASSIGN);
	if (assignment(&node->value.children.l[new_index]))
		return true;

	set_token(start_index);
	ast_remove_node(node, new_index);
	new_index = ast_insert_node(node, AST_FUNC_CALL);
	if (func_call(&node->value.children.l[new_index]))
		return true;

	set_token(start_index);
	ast_remove_node(node, new_index);
	new_index = ast_insert_node(node, AST_CONDITIONAL);
	if (conditional(&node->value.children.l[new_index]))
		return true;

	return false;
}

static bool statement_list(struct ast_node *node) {
	// TODO: consider empty statements ({})
	if (!is_type(LEX_LEFT_BRACE))
		return false;

	next();

	size_t new_index;
	do {
		new_index = ast_insert_node(node, AST_STMT);
	}
	while (statement(&node->value.children.l[new_index]));
	ast_remove_node(node, new_index);

	expect(LEX_RIGHT_BRACE);
	next();

	return true;
}

static bool conditional(struct ast_node *node) {
	if (!is_type(LEX_IF))
		return false;

	next();

	expect(LEX_LEFT_PAREN);
	next();

	size_t new_index = ast_insert_node(node, AST_EXPR);
	expression(&node->value.children.l[new_index]);

	expect(LEX_RIGHT_PAREN);
	next();

	new_index = ast_insert_node(node, AST_STMT_LIST);
	if (!statement_list(&node->value.children.l[new_index]))
		return false;

	if (is_type(LEX_ELSE)) {
		next();
		new_index = ast_insert_node(node, AST_STMT_LIST);
		if (!statement_list(&node->value.children.l[new_index]))
			return false;
	}

	return true;

}

static void goal(struct ast_node *node) {
	size_t new_index = ast_insert_node(node, AST_STMT_LIST);
	bool ok = statement_list(&node->value.children.l[new_index]);

	// size_t new_index = ast_insert_node(node, AST_EXPR);
	// expression(&node->value.children.l[new_index]);

	printf("success? %u\n", ok);
}

void parse(const struct lex_token_list *tokens, struct ast_node *root) {
	token_list = *tokens;
	current_index = 0;
	goal(root);
}


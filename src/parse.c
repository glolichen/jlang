#include <stdio.h>
#include <stdbool.h>

#include "lex.h"
#include "parse.h"
#include "ast.h"

#include "setjmp.h"

jmp_buf error_buf;

static const enum lex_token_type COMP_OPS[] = {
	LEX_EQUAL_EQUAL, LEX_BANG_EQUAL,
	LEX_LESS, LEX_LESS_EQUAL,
	LEX_GREATER, LEX_GREATER_EQUAL
};
static const size_t COMP_OPS_SIZE = sizeof(COMP_OPS) / sizeof(COMP_OPS[0]);

static const char **line_list;
static struct lex_token_list token_list;
static size_t current_index = 0;

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

static void print_cur_no_prefix(FILE *out) {
	size_t line_num = get_cur()->line - 1;
	const char *line = line_list[line_num];
	size_t i = 0;
	while (true) {
		if (line[i] == 0)
			break;
		if (line[i] != ' ' && line[i] != '\t')
			break;
		i++;
	}
	fprintf(out, "%s\n", line + i);
}

// check if current lexeme is OK (in the list)
static bool is_type(enum lex_token_type type) {
	if (current_index >= token_list.size)
		return false;
	return token_list.l[current_index].type == type;
}
static bool is_types(const enum lex_token_type *type, size_t amount) {
	if (current_index >= token_list.size)
		return false;
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

	const struct lex_token *token = get_cur();
	fprintf(
		stderr,
		"[ERROR] expected %s, got %s (\"%s\")\n",
		lex_token_type_to_str(type),
		lex_token_type_to_str(token->type),
		token->str
	);
	fprintf(stderr, "line %zu: ", token->line);
	print_cur_no_prefix(stderr);
	longjmp(error_buf, 1);
}

static void expr_no_comp(struct ast_node *node);
static void expression(struct ast_node *node);

static bool func_call(struct ast_node *node);

static void factor(struct ast_node *node) {
	size_t start_index = current_index;

	size_t new_index = ast_insert_node(node, AST_FUNC_CALL);
	if (func_call(&node->value.children.l[new_index])) 
		return;

	set_token(start_index);
	ast_remove_node(node, new_index);

	if (is_type(LEX_NUMBER) || is_type(LEX_IDENTIFIER)) {
		ast_insert_leaf(node, get_cur());
		next();
		return;
	}
	if (is_type(LEX_LEFT_PAREN)) {
		next();

		size_t new_index = ast_insert_node(node, AST_EXPR);
		expression(&node->value.children.l[new_index]);

		expect(LEX_RIGHT_PAREN);

		next();

		return;
	}

	fprintf(stderr, "[ERROR] invalid expression\n");
	fprintf(stderr, "line %zu: ", get_cur()->line);
	print_cur_no_prefix(stderr);
	longjmp(error_buf, 1);
}

static void term(struct ast_node *node) {
	size_t new_index = ast_insert_node(node, AST_FACTOR);
	factor(&node->value.children.l[new_index]);

	while (is_type(LEX_STAR) || is_type(LEX_SLASH) || is_type(LEX_PERCENT)) {
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

	if (!is_types(COMP_OPS, COMP_OPS_SIZE))
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
	next();

	size_t new_index = ast_insert_node(node, AST_EXPR);
	expression(&node->value.children.l[new_index]);

	return true;
}

static bool func_call(struct ast_node *node) {
	if (!is_type(LEX_IDENTIFIER))
		return false;

	ast_insert_leaf(node, get_cur());
	next();

	size_t new_index = ast_insert_node(node, AST_EXPR_LIST);
	if (!expression_list(&node->value.children.l[new_index])) {
		return false;
		// fprintf(stderr, "[ERROR] expected expression list in function call\n");
		// fprintf(stderr, "line %zu: ", get_cur()->line);
		// print_cur_no_prefix(stderr);
		// longjmp(error_buf, 1);
	}

	return true;
}

static bool parse_return(struct ast_node *node) {
	if (!is_type(LEX_RETURN))
		return false;

	next();

	size_t new_index = ast_insert_node(node, AST_EXPR);
	expression(&node->value.children.l[new_index]);

	return true;
}

static bool conditional(struct ast_node *node);
static bool for_loop(struct ast_node *node);

static bool continue_break(struct ast_node *node) {
	if (is_type(LEX_CONTINUE)) {
		ast_insert_node(node, AST_CONTINUE);
		next();
		return true;
	}
	if (is_type(LEX_BREAK)) {
		ast_insert_node(node, AST_BREAK);
		next();
		return true;
	}
	return false;
}

static bool statement(struct ast_node *node) {
	if (is_type(LEX_SEMICOLON)) {
		next();
		return true;
	}

	size_t start_index = current_index;

	size_t new_index = ast_insert_node(node, AST_ASSIGN);
	if (assignment(&node->value.children.l[new_index])) {
		expect(LEX_SEMICOLON);
		next();
		return true;
	}

	set_token(start_index);
	ast_remove_node(node, new_index);
	new_index = ast_insert_node(node, AST_FUNC_CALL);
	if (func_call(&node->value.children.l[new_index])) {
		expect(LEX_SEMICOLON);
		next();
		return true;
	}

	set_token(start_index);
	ast_remove_node(node, new_index);
	new_index = ast_insert_node(node, AST_CONDITIONAL);
	if (conditional(&node->value.children.l[new_index]))
		return true;

	set_token(start_index);
	ast_remove_node(node, new_index);
	new_index = ast_insert_node(node, AST_FOR);
	if (for_loop(&node->value.children.l[new_index]))
		return true;

	set_token(start_index);
	ast_remove_node(node, new_index);
	new_index = ast_insert_node(node, AST_RETURN);
	if (parse_return(&node->value.children.l[new_index])) {
		expect(LEX_SEMICOLON);
		next();
		return true;
	}

	set_token(start_index);
	ast_remove_node(node, new_index);
	if (continue_break(node)) {
		expect(LEX_SEMICOLON);
		next();
		return true;
	}

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
	} while (statement(&node->value.children.l[new_index]));
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

static bool for_loop(struct ast_node *node) {
	if (!is_type(LEX_FOR))
		return false;

	next();

	expect(LEX_LEFT_PAREN);
	next();

	size_t new_index = ast_insert_node(node, AST_ASSIGN);
	if (!is_type(LEX_SEMICOLON)) {
		if (!assignment(&node->value.children.l[new_index])) 
			return false;
		expect(LEX_SEMICOLON);
	}

	next();

	new_index = ast_insert_node(node, AST_EXPR);
	if (!is_type(LEX_SEMICOLON)) {
		expression(&node->value.children.l[new_index]);
		expect(LEX_SEMICOLON);
	}

	next();

	new_index = ast_insert_node(node, AST_ASSIGN);
	if (!is_type(LEX_RIGHT_PAREN)) {
		if (!assignment(&node->value.children.l[new_index])) 
			return false;
		expect(LEX_RIGHT_PAREN);
	}

	next();

	new_index = ast_insert_node(node, AST_STMT_LIST);
	if (!statement_list(&node->value.children.l[new_index]))
		return false;

	return true;
}

static void goal(struct ast_node *node) {
	// size_t new_index = ast_insert_node(node, AST_EXPR);
	// expression(&node->value.children.l[new_index]);

	size_t new_index = ast_insert_node(node, AST_STMT_LIST);
	bool ok = statement_list(&node->value.children.l[new_index]);

	printf("success? %u\n", ok);
}

bool parse(const struct lex_token_list *tokens, const char **lines, struct ast_node *root) {
	current_index = 0;
	token_list = *tokens, line_list = lines;
    if (!setjmp(error_buf)) {
		goal(root);
		return true;
	}
    else
		return false;
}


#ifndef LEX_H
#define LEX_H

#include <stdlib.h>

// https://craftinginterpreters.com/scanning.html
enum lex_token_type {
	LEX_LEFT_PAREN, LEX_RIGHT_PAREN, LEX_LEFT_BRACE, LEX_RIGHT_BRACE,
	LEX_COMMA, LEX_DOT, LEX_MINUS, LEX_PLUS, LEX_SEMICOLON, LEX_SLASH, LEX_STAR,

	// One or two character tokens.
	LEX_BANG, LEX_BANG_EQUAL,
	LEX_EQUAL, LEX_EQUAL_EQUAL,
	LEX_GREATER, LEX_GREATER_EQUAL,
	LEX_LESS, LEX_LESS_EQUAL,

	// Literals.
	LEX_IDENTIFIER, LEX_STRING, LEX_NUMBER,

	LEX_AND, LEX_ELSE, LEX_FALSE, LEX_FOR, LEX_IF, LEX_OR,
	LEX_RETURN, LEX_TRUE, LEX_WHILE,

	LEX_INT, LEX_CHAR,

	LEX_NOTHING,
};
const char *lex_token_type_to_str(enum lex_token_type type);

union lex_token_literal {
	int number;
	const char *string;
};

struct lex_token {
	enum lex_token_type type;
	union lex_token_literal literal;
	const char *str;
	int line;
};

struct lex_token_list {
	struct lex_token *l;
	size_t size, capacity;
};

struct lex_scan_error {
	const char *msg;
	int line;
};

struct lex_token lex_new_token(enum lex_token_type type, const char *str, int line);
void lex_free_token(struct lex_token *token);

struct lex_token_list lex_new_token_list(void);
void lex_free_token_list(struct lex_token_list *list);

struct lex_scan_error lex_scan(size_t num_lines, const char **lines, const size_t *line_lens, struct lex_token_list *token_list);
void lex_print_token(const struct lex_token *token);

#endif

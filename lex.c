#include <stdio.h>
#include <string.h>

#include "lex.h"

static inline size_t min(size_t a, size_t b) {
	return a < b ? a : b;
}

static const char *token_type_to_str(enum lex_token_type type) {
	switch (type) {
		case LEX_LEFT_PAREN:
			return "LEFT_PAREN";
		case LEX_RIGHT_PAREN:
			return "RIGHT_PAREN";
		case LEX_LEFT_BRACE:
			return "LEFT_BRACE";
		case LEX_RIGHT_BRACE:
			return "RIGHT_BRACE";
		case LEX_COMMA:
			return "COMMA";
		case LEX_DOT:
			return "DOT";
		case LEX_MINUS:
			return "MINUS";
		case LEX_PLUS:
			return "PLUS";
		case LEX_SEMICOLON:
			return "SEMICOLON";
		case LEX_SLASH:
			return "SLASH";
		case LEX_STAR:
			return "STAR";
		case LEX_BANG:
			return "BANG";
		case LEX_BANG_EQUAL:
			return "BANG_EQUAL";
		case LEX_EQUAL:
			return "EQUAL";
		case LEX_EQUAL_EQUAL:
			return "EQUAL_EQUAL";
		case LEX_GREATER:
			return "GREATER";
		case LEX_GREATER_EQUAL:
			return "GREATER_EQUAL";
		case LEX_LESS:
			return "LESS";
		case LEX_LESS_EQUAL:
			return "LESS_EQUAL";
		case LEX_IDENTIFIER:
			return "IDENTIFIER";
		case LEX_STRING:
			return "STRING";
		case LEX_NUMBER:
			return "NUMBER";
		case LEX_AND:
			return "AND";
		case LEX_ELSE:
			return "ELSE";
		case LEX_FALSE:
			return "FALSE";
		case LEX_FOR:
			return "FOR";
		case LEX_IF:
			return "IF";
		case LEX_OR:
			return "OR";
		case LEX_RETURN:
			return "RETURN";
		case LEX_TRUE:
			return "TRUE";
		case LEX_WHILE:
			return "WHILE";
		case LEX_INT:
			return "INT";
		case LEX_CHAR:
			return "CHAR";
		case LEX_NOTHING:
			return "NOTHING";
	}
	return "";
}

static enum lex_token_type str_to_keyword(const char *str) {
	if (strcmp(str, "else") == 0)
		return LEX_ELSE;
	if (strcmp(str, "false") == 0)
		return LEX_FALSE;
	if (strcmp(str, "for") == 0)
		return LEX_FOR;
	if (strcmp(str, "if") == 0)
		return LEX_IF;
	if (strcmp(str, "return") == 0)
		return LEX_RETURN;
	if (strcmp(str, "true") == 0)
		return LEX_TRUE;
	if (strcmp(str, "while") == 0)
		return LEX_WHILE;
	if (strcmp(str, "int") == 0)
		return LEX_INT;
	if (strcmp(str, "char") == 0)
		return LEX_CHAR;
	return -1;
}

static enum lex_token_type str_to_delim(const char *str) {
	if (strcmp(str, " "))
		return LEX_NOTHING;
	if (strcmp(str, "\t"))
		return LEX_NOTHING;
	if (strcmp(str, "\n"))
		return LEX_NOTHING;
	if (strcmp(str, "("))
		return LEX_LEFT_PAREN;
	if (strcmp(str, ")"))
		return LEX_RIGHT_PAREN;
	if (strcmp(str, "{"))
		return LEX_LEFT_BRACE;
	if (strcmp(str, "}"))
		return LEX_RIGHT_BRACE;
	if (strcmp(str, ","))
		return LEX_COMMA;
	if (strcmp(str, "."))
		return LEX_DOT;
	if (strcmp(str, "-"))
		return LEX_MINUS;
	if (strcmp(str, "+"))
		return LEX_PLUS;
	if (strcmp(str, ";"))
		return LEX_SEMICOLON;
	if (strcmp(str, "/"))
		return LEX_SLASH;
	if (strcmp(str, "*"))
		return LEX_STAR;
	if (strcmp(str, "!"))
		return LEX_BANG;
	if (strcmp(str, "!="))
		return LEX_BANG_EQUAL;
	if (strcmp(str, "="))
		return LEX_EQUAL;
	if (strcmp(str, "=="))
		return LEX_EQUAL_EQUAL;
	if (strcmp(str, ">"))
		return LEX_GREATER;
	if (strcmp(str, ">="))
		return LEX_GREATER_EQUAL;
	if (strcmp(str, "<"))
		return LEX_LESS;
	if (strcmp(str, "<="))
		return LEX_LESS_EQUAL;
	if (strcmp(str, "&&"))
		return LEX_AND;
	if (strcmp(str, "||"))
		return LEX_OR;
	return -1;
}


// WARN: remember to update these
static const size_t max_keyword_len = 6;
static const size_t max_delim_len = 2;

struct lex_token_list lex_new_token_list(void) {
	struct lex_token_list list;
	list.l = NULL, list.size = 0, list.capacity = 0;
	return list;
}
void lex_token_list_append(struct lex_token_list *list, struct lex_token token) {
	if (list->capacity == 0) {
		list->capacity = 1, list->size = 1;
		list->l = malloc(1 * sizeof(struct lex_token));
		list->l[0] = token;
		return;
	}
	if (list->size + 1 <= list->capacity) {
		list->l[list->size++] = token;
		return;
	}

	list->capacity *= 2;
	list->l = realloc(list->l, list->capacity * sizeof(struct lex_token));
	list->l[list->size++] = token;
}
void lex_free_token_list(struct lex_token_list *list) {
	free(list->l);
	list->capacity = 0, list->size = 0;
}

struct lex_scan_error create_scan_error(const char *msg, int line) {
	struct lex_scan_error error;
	error.msg = msg;
	error.line = line;
	return error;
}
struct lex_token create_token(enum lex_token_type type, const char *str, int line) {
	struct lex_token token;
	token.type = type;
	token.literal_type = TOKEN_LITERAL_NONE;
	token.literal.number = 0;
	token.str = str;
	token.line = line;
	return token;
}

static const char *scan_line(int line_num, struct lex_token_list *token_list, const char *line, size_t line_len) {
	size_t start = 0;
	for (size_t i = 0; i < line_len; i++) {
		for (size_t j = min(max_delim_len, line_len - i); j >= 1; j--) {
			char *delim = malloc((j + 1) * sizeof(char));
			strncpy(delim, line + i, j);
			delim[j] = 0;

			enum lex_token_type delim_found = str_to_delim(delim);
			if ((int) delim_found == -1) {
				free(delim);
				continue;
			}

			if (start != i) {
				char *prev_token = malloc((i - start + 1) * sizeof(char));
				strncpy(prev_token, line + start, i - start);
				prev_token[i - start] = 0;

				enum lex_token_type kw_found = str_to_keyword(prev_token);
				enum lex_token_type type = (int) kw_found == -1 ? LEX_IDENTIFIER : kw_found;

				lex_token_list_append(token_list, create_token(type, prev_token, line_num));
			}

			if (delim_found != LEX_NOTHING)
				lex_token_list_append(token_list, create_token(delim_found, delim, line_num));
			else
				free(delim);

			start = i + j;
			i += j - 1;

			break;
		}
	}

	// for (size_t i = 0; i < token_list->size; i++) {
	//
	// }
	//
	// for (lex::Token &token : tokens) {
	// 	int num;
	// 	try {
	// 		num = std::stoi(token.str);
	// 	}
	//        catch (const std::invalid_argument &ex) {
	// 		continue;
	//        }
	//        catch (const std::out_of_range &ex) {
	// 		return { lex::SCAN_LINE_INT_OUT_OF_BOUNDS, token.line };
	// 	}
	// 	token.type = lex::NUMBER;
	// 	token.literal = num;
	// }

	// return { lex::SCAN_LINE_OK, 0 };

	return "";
}

struct lex_scan_error lex_scan(size_t num_lines, const char **lines, const size_t *line_lens, struct lex_token_list *token_list) {
	for (size_t i = 0; i < num_lines; i++) {
		scan_line(i + 1, token_list, lines[i], line_lens[i]);
		// if (ret.type != lex::SCAN_LINE_OK)
		// 	return ret;
	}
	return create_scan_error("", 0);
}

void lex_print_token(const struct lex_token *token) {
	printf("{ type = %s, literal = ", token_type_to_str(token->type));
	if (token->type == LEX_NUMBER)
		printf("%d", token->literal.number);
	else if (token->type == LEX_STRING)
		printf("%s", token->literal.string);
	else
		printf("[none]");
	printf(", str = \"%s\", line = %d }", token->str, token->line);
}

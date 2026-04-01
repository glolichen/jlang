#ifndef PARSE_H
#define	PARSE_H

#include "lex.h"
#include "ast.h"

bool parse(const struct lex_token_list *tokens, const char **lines, struct ast_node *root);

#endif


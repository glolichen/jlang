#ifndef PARSE_H
#define	PARSE_H

#include "lex.h"
#include "ast.h"

void parse(const struct lex_token_list *tokens, struct ast_node *root);

#endif


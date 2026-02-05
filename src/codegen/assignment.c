#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "codegen/assignment.h"
#include "codegen/expression.h"
#include "strmap.h"
#include "ast.h"
#include "lex.h"

void codegen_assignment(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_ASSIGN) {
		fprintf(stderr, "ERROR! (12)\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;

	if (list->size != 2 || list->l[0].type != AST_LEAF) {
		fprintf(stderr, "ERROR! (13)\n");
		exit(1);
	}

	const struct lex_token *ident = &list->l[0].value.token;
	LLVMValueRef rhs = codegen_expression(build, &list->l[1], var_map, func_map);
	strmap_set(var_map, ident->str, &rhs, sizeof(LLVMValueRef));
}


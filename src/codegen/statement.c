#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen/function.h"
#include "codegen/return.h"
#include "codegen/forloop.h"
#include "codegen/assignment.h"
#include "codegen/conditional.h"
#include "strmap.h"
#include "ast.h"

void codegen_statement(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_STMT) {
		fprintf(stderr, "ERROR! (16)\n");
		exit(1);
	}

	const struct ast_node *child = &node->value.children.l[0];
	switch (child->type) {
		case AST_ASSIGN:
			codegen_assignment(build, child, var_map, func_map);
			break;	
		case AST_RETURN:
			codegen_return(build, child, var_map, func_map);
			break;
		case AST_FUNC_CALL:
			codegen_func_call(build, child, var_map, func_map);
			break;
		case AST_CONDITIONAL:
			codegen_conditional(build, child, var_map, func_map);
			break;
		case AST_FOR:
			codegen_for_loop(build, child, var_map, func_map);
			break;
		// loops have custom handling for continue/break, do not use this function
		case AST_CONTINUE:
		case AST_BREAK:
			fprintf(stderr, "ERROR! continue/break outside loop\n");
			exit(1);
		default:
			fprintf(stderr, "ERROR! (17)\n");
			exit(1);
	}
}

void codegen_stmt_list(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_STMT_LIST) {
		fprintf(stderr, "ERROR! (18)\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;
	for (size_t i = 0; i < list->size; i++)
		codegen_statement(build, &list->l[i], var_map, func_map);
}


#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen/function.h"
#include "codegen/expression.h"
#include "codegen/return.h"
#include "codegen/forloop.h"
#include "codegen/assignment.h"
#include "codegen/conditional.h"
#include "utils/strmap.h"
#include "ast.h"

// return whether to continue generating code
// (after continue/break/return, stop generating so LLVM doesn't complain)
bool codegen_statement(
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
			codegen_continue(build, var_map, func_map);
			return true;
		case AST_BREAK:
			codegen_break(build, var_map);
			return true;
		default:
			fprintf(stderr, "ERROR! (17)\n");
			exit(1);
	}

	return false;
}

// returns true if there is a terminator (continue/break/return) in this list
bool codegen_stmt_list(
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
	for (size_t i = 0; i < list->size; i++) {
		// if any is terminated, stop generating
		if (codegen_statement(build, &list->l[i], var_map, func_map))
			return true;
	}

	return false;
}


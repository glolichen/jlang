#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen/function.h"
#include "codegen/forloop.h"
#include "codegen/variable.h"
#include "codegen/conditional.h"
#include "utils/strmap.h"
#include "error.h"
#include "ast.h"

// return whether to continue generating code
// avoid generating a temrinator in the middle of a basic block
// (after continue/break/return, stop generating so LLVM doesn't complain)
bool codegen_statement(
	LLVMBuilderRef build,
	struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->node_type != AST_STMT)
		ERROR_COMPILER();

	struct ast_node *child = &node->value.children.l[0];
	switch (child->node_type) {
		case AST_ASSIGN:
			codegen_assignment(build, child, var_map, func_map);
			return false;
		case AST_VAR_DECLARATION:
			codegen_var_declaration(build, child, var_map, func_map);
			return false;
		case AST_FUNC_CALL:
			codegen_func_call(build, child, var_map, func_map);
			return false;
		case AST_CONDITIONAL:
			codegen_conditional(build, child, var_map, func_map);
			return false;
		case AST_FOR:
			codegen_for_loop(build, child, var_map, func_map);
			return false;
		case AST_RETURN:
			codegen_return(build, child, var_map, func_map);
			return true;
		case AST_CONTINUE:
			codegen_continue(build, child, var_map);
			return true;
		case AST_BREAK:
			codegen_break(build, child, var_map);
			return true;
		default:
			ERROR_COMPILER();
	}

	return false;
}

// returns true if there is a terminator (continue/break/return) in this list
bool codegen_stmt_list(
	LLVMBuilderRef build,
	struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->node_type != AST_STMT_LIST)
		ERROR_COMPILER();

	struct ast_node_list *list = &node->value.children;
	for (size_t i = 0; i < list->size; i++) {
		// if any is terminated, stop generating
		if (codegen_statement(build, &list->l[i], var_map, func_map)) {
			// TODO: If i != size - 1, then error
			return true;
		}
	}

	return false;
}


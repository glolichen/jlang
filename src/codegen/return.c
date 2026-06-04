#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <stdio.h>
#include <stdlib.h>

#include "codegen/return.h"
#include "codegen/expression.h"
#include "utils/strmap.h"
#include "error.h"
#include "ast.h"

void codegen_return(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->node_type != AST_RETURN)
		ERROR_COMPILER();

	const struct ast_node_list *list = &node->value.children;

	if (list->size != 1 || list->l[0].node_type != AST_EXPR)
		ERROR_COMPILER();

	LLVMValueRef value = codegen_expression(build, &list->l[0], var_map, func_map);
	LLVMBuildRet(build, value);
}


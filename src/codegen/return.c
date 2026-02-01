#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <stdio.h>
#include <stdlib.h>

#include "codegen/return.h"
#include "codegen/expression.h"
#include "strmap.h"
#include "ast.h"

void codegen_return(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_RETURN) {
		fprintf(stderr, "ERROR! (14)\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;

	if (list->size != 1 || list->l[0].type != AST_EXPR) {
		fprintf(stderr, "ERROR! (15)\n");
		exit(1);
	}

	LLVMValueRef value = codegen_expression(mod, build, &list->l[0], var_map, func_map);
	LLVMBuildRet(build, value);
}


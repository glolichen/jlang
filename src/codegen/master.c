#include <llvm-c/Core.h>
#include "codegen/master.h"
#include "codegen/decl_def.h"
#include "error.h"
#include "ast.h"

void codegen_master(
	LLVMModuleRef module,
	LLVMBuilderRef build,
	struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->node_type != AST_MASTER_LIST)
		ERROR_COMPILER();

	struct ast_node_list *list = &node->value.children;
	for (size_t i = 0; i < list->size; i++) {
		// if any is terminated, stop generating
		codegen_decl_def(module, build, &list->l[i], var_map, func_map);
	}
}

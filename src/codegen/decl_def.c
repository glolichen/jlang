#include <llvm-c/Core.h>

#include "codegen/function.h"
#include "utils/strmap.h"
#include "error.h"
#include "ast.h"

void codegen_decl_def(
	LLVMModuleRef module,
	LLVMBuilderRef build,
	struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	struct ast_node *child = &node->value.children.l[0];
	switch (child->node_type) {
		case AST_FUNC_DEFINITION:
			codegen_func_definition(module, build, child, var_map, func_map);
			break;	
		default:
			ERROR_COMPILER();
	}
}

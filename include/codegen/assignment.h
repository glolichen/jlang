#ifndef CODEGEN_ASSIGNMENT_H
#define CODEGEN_ASSIGNMENT_H

#include <llvm-c/Core.h>
#include "../strmap.h"
#include "../ast.h"

void codegen_assignment(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
);

#endif


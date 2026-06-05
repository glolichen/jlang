#ifndef CODEGEN_MASTER_H
#define CODEGEN_MASTER_H

#include <llvm-c/Core.h>
#include "utils/strmap.h"
#include "ast.h"

void codegen_master(
	LLVMModuleRef module,
	LLVMBuilderRef build,
	struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
);

#endif


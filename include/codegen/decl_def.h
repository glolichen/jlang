#ifndef CODEGEN_DECL_DEF_H
#define CODEGEN_DECL_DEF_H

#include <llvm-c/Core.h>
#include "utils/strmap.h"
#include "ast.h"

void codegen_decl_def(
	LLVMModuleRef module,
	LLVMBuilderRef build,
	struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
);

#endif


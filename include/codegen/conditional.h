#ifndef CODEGEN_CONDITIONAL_H
#define CODEGEN_CONDITIONAL_H

#include <llvm-c/Core.h>
#include "utils/strmap.h"
#include "ast.h"

void codegen_conditional(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
);

#endif

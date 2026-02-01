#ifndef CODEGEN_FUNCTION_H
#define CODEGEN_FUNCTION_H

#include <llvm-c/Core.h>
#include "../strmap.h"
#include "../ast.h"

void codegen_func_init(struct strmap *func_map);

LLVMValueRef codegen_func_call(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
);

#endif


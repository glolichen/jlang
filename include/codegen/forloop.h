#ifndef CODEGEN_FORLOOP_H
#define CODEGEN_FORLOOP_H

#include <llvm-c/Core.h>
#include "utils/strmap.h"
#include "ast.h"

void codegen_continue(
	LLVMBuilderRef build,
	struct strmap *var_map,
	struct strmap *func_map
);

void codegen_break(
	LLVMBuilderRef build,
	const struct strmap *var_map
);

void codegen_for_loop(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
);

#endif


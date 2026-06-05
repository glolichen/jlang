#ifndef CODEGEN_FORLOOP_H
#define CODEGEN_FORLOOP_H

#include <llvm-c/Core.h>
#include "utils/strmap.h"
#include "ast.h"

void codegen_continue(
	LLVMBuilderRef build,
	struct ast_node *node,
	struct strmap *var_map
);

void codegen_break(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map
);

void codegen_for_loop(
	LLVMBuilderRef build,
	struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
);

#endif


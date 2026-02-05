#ifndef CODEGEN_FORLOOP_H
#define CODEGEN_FORLOOP_H

#include <llvm-c/Core.h>
#include "../strmap.h"
#include "../ast.h"

struct codegen_for_loop_context {
	LLVMBasicBlockRef cond_block, after_block;
};

const struct codegen_for_loop_context *codegen_get_for_loop_context(void);

void codegen_for_loop(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
);

#endif


#ifndef CODEGEN_FORLOOP_H
#define CODEGEN_FORLOOP_H

#include <llvm-c/Core.h>
#include "utils/strmap.h"
#include "ast.h"

struct codegen_for_loop_context {
	LLVMBasicBlockRef body_block, after_phi_block;
	struct strmap *loop_phi_nodes;
	const struct ast_node *for_node;
};

const struct codegen_for_loop_context *codegen_get_for_loop_context(void);

void codegen_for_loop(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
);

#endif


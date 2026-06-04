#ifndef CODEGEN_VARIABLE_H
#define CODEGEN_VARIABLE_H

#include <llvm-c/Core.h>
#include "types.h"
#include "utils/strmap.h"
#include "ast.h"

struct var_map_entry {
	LLVMValueRef value;
	LLVMTypeRef type;
};

void codegen_var_declaration(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
);

void codegen_assignment(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
);

#endif


#ifndef CODEGEN_STATEMENT_H
#define CODEGEN_STATEMENT_H

#include <llvm-c/Core.h>
#include "utils/strmap.h"
#include "ast.h"

bool codegen_statement(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
);

bool codegen_stmt_list(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
);

#endif


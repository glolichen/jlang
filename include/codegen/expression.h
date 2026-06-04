#ifndef CODEGEN_EXPRESSION_H
#define CODEGEN_EXPRESSION_H

#include <llvm-c/Core.h>
#include "utils/strmap.h"
#include "ast.h"
#include "lex.h"

LLVMValueRef codegen_number(
	LLVMContextRef llvm_ctx,
	struct ast_node *node
);

LLVMValueRef codegen_factor(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
);

LLVMValueRef codegen_term(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
);

LLVMValueRef codegen_expr_no_comp(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
);

LLVMValueRef codegen_expression(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
);

#endif


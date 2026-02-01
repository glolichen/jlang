#ifndef CODEGEN_EXPRESSION_H
#define CODEGEN_EXPRESSION_H

#include <llvm-c/Core.h>
#include "../strmap.h"
#include "../ast.h"
#include "../lex.h"

LLVMValueRef codegen_number(const struct lex_token *token);
LLVMValueRef codegen_factor(
	LLVMModuleRef module, LLVMBuilderRef builder,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
);

LLVMValueRef codegen_term(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
);

LLVMValueRef codegen_expr_no_comp(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
);

LLVMValueRef codegen_expression(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
);

#endif


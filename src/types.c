#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <string.h>
#include <stdio.h>

#include "codegen/expression.h"
#include "types.h"
#include "error.h"
#include "ast.h"
#include "lex.h"

const char *value_type_to_str(LLVMTypeRef type, LLVMBuilderRef build) {
	if (type == TYPE_I8(build))
		return "i8";
	if (type == TYPE_I16(build))
		return "i16";
	if (type == TYPE_I32(build))
		return "i32";
	if (type == TYPE_I64(build))
		return "i64";
	if (type == NULL)
		return "UNKNOWN";
	ERROR_COMPILER();
}

LLVMValueRef types_convert_literal(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	const char *func_name = node->value.children.l[0].value.token.str;

	LLVMContextRef llvm_ctx = LLVMGetBuilderContext(build);

	LLVMTypeRef llvm_type;
	if (strcmp(func_name, "to_i8") == 0)
		llvm_type = LLVMIntTypeInContext(llvm_ctx, 8);
	else if (strcmp(func_name, "to_i16") == 0)
		llvm_type = LLVMIntTypeInContext(llvm_ctx, 16);
	else if (strcmp(func_name, "to_i32") == 0)
		llvm_type = LLVMIntTypeInContext(llvm_ctx, 32);
	else if (strcmp(func_name, "to_i64") == 0)
		llvm_type = LLVMIntTypeInContext(llvm_ctx, 64);
	else
		ERROR_COMPILER();

	struct ast_node_list *ast_params = &node->value.children.l[1].value.children;

	if (ast_params->size != 1)
		goto wrong_params;

	struct ast_node *expr = &ast_params->l[0];
	if (expr->node_type != AST_EXPR)
		goto wrong_params;

	node->value_type = llvm_type;

	LLVMValueRef expr_value = codegen_expression(build, expr, var_map, func_map);
	return LLVMBuildIntCast2(build, expr_value, llvm_type, true, "intcasttmp");

wrong_params:
	fprintf(
		stderr,
		"function %s accepts 1 number literal parameter\n",
		func_name
	);
	exit(1);
}


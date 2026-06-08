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

LLVMTypeRef type_from_lex(
	LLVMBuilderRef build,
	enum lex_token_type token,
	bool include_void
) {
	if (token == LEX_I8)
		return TYPE_I8(build);
	if (token == LEX_I16)
		return TYPE_I16(build);
	if (token == LEX_I32)
		return TYPE_I32(build);
	if (token == LEX_I64)
		return TYPE_I64(build);
	if (token == LEX_VOID && include_void)
		return TYPE_VOID(build);
	ERROR_COMPILER();
}

LLVMValueRef types_convert_literal(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	const char *func_name = node->value.children.l[0].value.token.str;

	LLVMTypeRef type;
	if (strcmp(func_name, "toi8") == 0)
		type = TYPE_I8(build);
	else if (strcmp(func_name, "toi16") == 0)
		type = TYPE_I16(build);
	else if (strcmp(func_name, "toi32") == 0)
		type = TYPE_I32(build);
	else if (strcmp(func_name, "toi64") == 0)
		type = TYPE_I64(build);
	else
		ERROR_COMPILER();

	struct ast_node_list *ast_params = &node->value.children.l[1].value.children;

	if (ast_params->size != 1)
		goto wrong_params;

	struct ast_node *expr = &ast_params->l[0];
	if (expr->node_type != AST_EXPR)
		goto wrong_params;

	node->value_type = type;

	LLVMValueRef expr_value = codegen_expression(build, expr, var_map, func_map);
	return LLVMBuildIntCast2(build, expr_value, type, true, "intcasttmp");

wrong_params:
	fprintf(
		stderr,
		"[ERROR] line %zu: function %s accepts 1 number literal parameter\n",
		node->line, func_name
	);
	exit(1);
}


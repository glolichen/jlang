#include <llvm-c/Core.h>
#include <string.h>
#include <stdio.h>

#include "codegen/types.h"
#include "error.h"
#include "ast.h"
#include "lex.h"

LLVMValueRef types_convert_literal(
	LLVMBuilderRef build,
	const struct ast_node *node
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
	if (expr->type != AST_EXPR)
		goto wrong_params;

	struct ast_node *expr_no_comp = &expr->value.children.l[0];
	if (expr_no_comp->type != AST_EXPR_NO_COMP)
		goto wrong_params;

	struct ast_node *term = &expr_no_comp->value.children.l[0];
	if (term->type != AST_TERM)
		goto wrong_params;

	struct ast_node *factor = &term->value.children.l[0];
	if (factor->type != AST_FACTOR)
		goto wrong_params;

	struct ast_node *leaf = &factor->value.children.l[0];
	if (leaf->type != AST_LEAF)
		goto wrong_params;

	struct lex_token *token = &leaf->value.token;
	if (token->type != LEX_NUMBER)
		goto wrong_params;

	int number = token->literal.number;

	return LLVMConstInt(llvm_type, number, 0);

wrong_params:
	fprintf(
		stderr,
		"function %s accepts 1 number literal parameter\n",
		func_name
	);
	exit(1);
}


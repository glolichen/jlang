#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <inttypes.h>
#include <stdio.h>

#include "codegen/variable.h"
#include "codegen/expression.h"
#include "utils/strmap.h"
#include "ast.h"
#include "error.h"
#include "lex.h"

void codegen_var_declaration(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_VAR_DECLARATION)
		ERROR_COMPILER();

	const struct ast_node_list *list = &node->value.children;

	if (
		list->size != 2 ||
		list->l[0].type != AST_LEAF ||
		list->l[1].type != AST_LEAF
	) {
		ERROR_COMPILER();
	}

	LLVMContextRef llvm_ctx = LLVMGetBuilderContext(build);

	const enum lex_token_type type = list->l[0].value.token.type;
	const struct lex_token *ident = &list->l[1].value.token;

	LLVMTypeRef llvm_type;
	switch (type) {
		case LEX_I8:
			llvm_type = LLVMIntTypeInContext(llvm_ctx, 8);
			break;
		case LEX_I16:
			llvm_type = LLVMInt16TypeInContext(llvm_ctx);
			break;
		case LEX_I32:
			llvm_type = LLVMInt32TypeInContext(llvm_ctx);
			break;
		case LEX_I64:
			llvm_type = LLVMInt64TypeInContext(llvm_ctx);
			break;
		default:
			ERROR_COMPILER();
	}

	LLVMValueRef rhs = LLVMConstInt(llvm_type, 0, 0);

	strmap_set(var_map, ident->str, &rhs, sizeof(LLVMValueRef));
}

void codegen_assignment(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_ASSIGN)
		ERROR_COMPILER();

	const struct ast_node_list *list = &node->value.children;

	if (list->size != 2 || list->l[0].type != AST_LEAF)
		ERROR_COMPILER();

	const struct lex_token *ident = &list->l[0].value.token;
	LLVMValueRef rhs = codegen_expression(build, &list->l[1], var_map, func_map);

	void *entry = strmap_get(var_map, ident->str);
	if (entry == NULL) {
		fprintf(
			stderr,
			"variable %s is not declared",
			ident->str
		);
		exit(1);
	}
	*(LLVMValueRef *) entry = rhs;
}


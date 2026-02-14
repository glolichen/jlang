#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen/expression.h"
#include "codegen/function.h"
#include "utils/strmap.h"
#include "ast.h"
#include "lex.h"

LLVMValueRef codegen_number(const struct lex_token *token) {
	return LLVMConstInt(LLVMInt32Type(), token->literal.number, 0);
}

LLVMValueRef codegen_factor(
	LLVMBuilderRef build,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	const struct ast_node *child = &node->value.children.l[0];

	if (child->type == AST_EXPR)
		return codegen_expression(build, child, var_map, func_map);

	if (child->type == AST_LEAF) {
		if (child->value.token.type == LEX_NUMBER)
			return codegen_number(&child->value.token);

		if (child->value.token.type == LEX_IDENTIFIER) {
			LLVMValueRef *value = strmap_get(var_map, child->value.token.str);
			if (value != NULL)
				return *value;
		}
	}
	else if (child->type == AST_FUNC_CALL) {
		LLVMValueRef value = codegen_func_call(build, child, var_map, func_map);
		if (value != NULL)
			return value;
	}

	fprintf(stderr, "ERROR! (1)\n");
	exit(1);
}

LLVMValueRef codegen_term(
	LLVMBuilderRef build,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_TERM) {
		fprintf(stderr, "ERROR! (2)\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;
	LLVMValueRef lhs = codegen_factor(build, &list->l[0], var_map, func_map);

	size_t i;
	for (i = 1; i < list->size - 1; i += 2) {
		if (list->l[i].type != AST_LEAF) {
			fprintf(stderr, "ERROR! (3)");
			exit(1);
		}

		LLVMValueRef rhs = codegen_factor(build, &list->l[i + 1], var_map, func_map);

		switch (list->l[i].value.token.type) {
			case LEX_STAR:
				lhs = LLVMBuildMul(build, lhs, rhs, "multmp");
				break;
			case LEX_SLASH:
				lhs = LLVMBuildSDiv(build, lhs, rhs, "divtmp");
				break;
			case LEX_PERCENT:
				lhs = LLVMBuildSRem(build, lhs, rhs, "modtmp");
				break;
			default:
				fprintf(stderr, "ERROR! (4)");
				exit(1);
		}
	}

	if (i != list->size) {
		fprintf(stderr, "ERROR! (5)");
		exit(1);
	}

	return lhs;
}

LLVMValueRef codegen_expr_no_comp(
	LLVMBuilderRef build,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_EXPR_NO_COMP) {
		fprintf(stderr, "ERROR! (6)\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;

	bool first_is_negative = false;
	size_t i = 0;
	if (list->l[0].type == AST_LEAF) {
		if (list->l[0].value.token.type == LEX_MINUS)
			first_is_negative = true;

		i++;
	}

	LLVMValueRef lhs = codegen_term(build, &list->l[i], var_map, func_map);
	if (first_is_negative)
		lhs = LLVMBuildMul(build, lhs, LLVMConstInt(LLVMInt32Type(), -1, 0), "negtmp");

	for (i = i + 1; i < list->size - 1; i += 2) {
		if (list->l[i].type != AST_LEAF) {
			fprintf(stderr, "ERROR! (7)");
			exit(1);
		}

		LLVMValueRef rhs = codegen_term(build, &list->l[i + 1], var_map, func_map);

		if (list->l[i].value.token.type == LEX_PLUS)
			lhs = LLVMBuildAdd(build, lhs, rhs, "addtmp");
		else if (list->l[i].value.token.type == LEX_MINUS)
			lhs = LLVMBuildSub(build, lhs, rhs, "subtmp");
	}

	if (i != list->size) {
		fprintf(stderr, "ERROR! (8)");
		exit(1);
	}

	return lhs;
}

LLVMValueRef codegen_expression(
	LLVMBuilderRef build,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_EXPR) {
		fprintf(stderr, "ERROR! (9)\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;

	// not comparison, only child is expr_no_comp
	if (list->size == 1)
		return codegen_expr_no_comp(build, &list->l[0], var_map, func_map);

	if (list->size == 3) {
		LLVMValueRef lhs = codegen_expr_no_comp(build, &list->l[0], var_map, func_map);
		enum lex_token_type comp_type = list->l[1].value.token.type;
		LLVMValueRef rhs = codegen_expr_no_comp(build, &list->l[2], var_map, func_map);

		LLVMIntPredicate comp_pred;
		switch (comp_type) {
			case LEX_GREATER:
				comp_pred = LLVMIntSGT;
				break;
			case LEX_GREATER_EQUAL:
				comp_pred = LLVMIntSGE;
				break;
			case LEX_LESS:
				comp_pred = LLVMIntSLT;
				break;
			case LEX_LESS_EQUAL:
				comp_pred = LLVMIntSLE;
				break;
			case LEX_EQUAL_EQUAL:
				comp_pred = LLVMIntEQ;
				break;
			case LEX_BANG_EQUAL:
				comp_pred = LLVMIntNE;
				break;
			default:
				fprintf(stderr, "ERROR! (10)");
				exit(1);
		}

		LLVMValueRef bool_value = LLVMBuildICmp(build, comp_pred, lhs, rhs, "cmptmp");
		return LLVMBuildIntCast2(build, bool_value, LLVMInt32Type(), false, "cmptmp2");
	}

	fprintf(stderr, "ERROR! (11)");
	exit(1);
}


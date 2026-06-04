#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen/expression.h"
#include "codegen/function.h"
#include "codegen/variable.h"
#include "utils/strmap.h"
#include "types.h"
#include "error.h"
#include "ast.h"
#include "lex.h"

LLVMValueRef codegen_number(
	LLVMContextRef llvm_ctx,
	struct ast_node *node
) {
	node->value_type = TYPE_I64_CTX(llvm_ctx);
	return LLVMConstInt(
		TYPE_I64_CTX(llvm_ctx),
		node->value.token.literal.number, 0
	);
}

LLVMValueRef codegen_factor(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	struct ast_node *child = &node->value.children.l[0];

	if (child->node_type == AST_EXPR) {
		LLVMValueRef ret = codegen_expression(build, child, var_map, func_map);
		node->value_type = child->value_type;
		return ret;
	}

	if (child->node_type == AST_LEAF) {
		if (child->value.token.type == LEX_NUMBER) {
			LLVMValueRef ret =  codegen_number(
				LLVMGetBuilderContext(build),
				child
			);
			node->value_type = child->value_type;
			return ret;
		}

		if (child->value.token.type == LEX_IDENTIFIER) {
			struct var_map_entry *value = strmap_get(var_map, child->value.token.str);
			if (value != NULL) {
				node->value_type = value->type;
				return value->value;
			}
		}
	}
	else if (child->node_type == AST_FUNC_CALL) {
		LLVMValueRef value = codegen_func_call(build, child, var_map, func_map);
		if (value != NULL) {
			node->value_type = child->value_type;
			return value;
		}
	}

	ERROR_COMPILER();
}

LLVMValueRef codegen_term(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->node_type != AST_TERM)
		ERROR_COMPILER();

	struct ast_node_list *list = &node->value.children;
	LLVMValueRef lhs = codegen_factor(build, &list->l[0], var_map, func_map);
	LLVMTypeRef type = list->l[0].value_type;

	size_t i;
	for (i = 1; i < list->size - 1; i += 2) {
		if (list->l[i].node_type != AST_LEAF)
			ERROR_COMPILER();

		LLVMValueRef rhs = codegen_factor(build, &list->l[i + 1], var_map, func_map);
		if (list->l[i + 1].value_type != type) {
			fprintf(
				stderr,
				"line %zu: type mismatch: expected %s, got %s\n",
				node->line,
				value_type_to_str(type, build),
				value_type_to_str(list->l[i + 1].value_type, build)
			);
			exit(1);
		}

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
				ERROR_COMPILER();
		}
	}

	if (i != list->size)
		ERROR_COMPILER();

	node->value_type = type;
	return lhs;
}

LLVMValueRef codegen_expr_no_comp(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->node_type != AST_EXPR_NO_COMP)
		ERROR_COMPILER();

	struct ast_node_list *list = &node->value.children;

	bool first_is_negative = false;
	size_t i = 0;
	if (list->l[0].node_type == AST_LEAF) {
		if (list->l[0].value.token.type == LEX_MINUS)
			first_is_negative = true;
		i++;
	}

	LLVMValueRef lhs = codegen_term(build, &list->l[i], var_map, func_map);
	LLVMTypeRef type = list->l[i].value_type;

	// WARN: possible that needs different integer type depending on type
	if (first_is_negative) {
		lhs = LLVMBuildMul(
			build, lhs,
			LLVMConstInt(
				LLVMInt32TypeInContext(LLVMGetBuilderContext(build)),
				-1, 0
			),
			"negtmp"
		);
	}

	for (i = i + 1; i < list->size - 1; i += 2) {
		if (list->l[i].node_type != AST_LEAF)
			ERROR_COMPILER();

		LLVMValueRef rhs = codegen_term(build, &list->l[i + 1], var_map, func_map);
		if (list->l[i + 1].value_type != type) {
			fprintf(
				stderr,
				"line %zu: type mismatch: expected %s, got %s\n",
				node->line,
				value_type_to_str(type, build),
				value_type_to_str(list->l[i + 1].value_type, build)
			);
			exit(1);
		}

		if (list->l[i].value.token.type == LEX_PLUS)
			lhs = LLVMBuildAdd(build, lhs, rhs, "addtmp");
		else if (list->l[i].value.token.type == LEX_MINUS)
			lhs = LLVMBuildSub(build, lhs, rhs, "subtmp");
	}

	if (i != list->size)
		ERROR_COMPILER();

	node->value_type = type;
	return lhs;
}

LLVMValueRef codegen_expression(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->node_type != AST_EXPR)
		ERROR_COMPILER();

	struct ast_node_list *list = &node->value.children;

	// not comparison, only child is expr_no_comp
	if (list->size == 1) {
		LLVMValueRef ret = codegen_expr_no_comp(build, &list->l[0], var_map, func_map);
		node->value_type = list->l[0].value_type;
		return ret;
	}

	if (list->size != 3)
		ERROR_COMPILER();

	LLVMValueRef lhs = codegen_expr_no_comp(build, &list->l[0], var_map, func_map);
	enum lex_token_type comp_type = list->l[1].value.token.type;
	LLVMValueRef rhs = codegen_expr_no_comp(build, &list->l[2], var_map, func_map);

	if (list->l[0].value_type != list->l[2].value_type) {
		fprintf(
			stderr,
			"line %zu: type mismatch: expected %s, got %s\n",
			node->line,
			value_type_to_str(list->l[0].value_type, build),
			value_type_to_str(list->l[2].value_type, build)
		);
		exit(1);
	}

	LLVMTypeRef type = list->l[0].value_type;

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
			ERROR_COMPILER();
	}

	LLVMValueRef bool_value = LLVMBuildICmp(build, comp_pred, lhs, rhs, "cmptmp");
	return LLVMBuildIntCast2(build, bool_value, type, false, "cmptmp2");
}


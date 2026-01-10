#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include <inttypes.h>
#include <llvm-c/Types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "strmap.h"
#include "ast.h"
#include "lex.h"

LLVMModuleRef module;
LLVMBuilderRef builder;

static struct {
	const char *func_name;
	LLVMValueRef func;
	bool is_defined;
} builtin_funcs[] = {
	{ "getchar", NULL, 0 },
};

// -1 = not builtin func, 0 = not defined, 1 = defined
static bool get_builtin_func(const char *func_name, bool *defined, LLVMValueRef *func, LLVMTypeRef *type) {
	size_t num_funcs = sizeof(builtin_funcs) / sizeof(builtin_funcs[0]);
	for (size_t i = 0; i < num_funcs; i++) {
		if (strcmp(builtin_funcs[i].func_name, func_name) == 0) {
			*defined = builtin_funcs[i].is_defined;
			*func = builtin_funcs[i].func;
			*type = LLVMFunctionType(LLVMInt8Type(), NULL, 0, 0);
			return true;
		}
	}
	return false;
}
static bool set_builtin_func(const char *func_name, bool value, LLVMValueRef func) {
	size_t num_funcs = sizeof(builtin_funcs) / sizeof(builtin_funcs[0]);
	for (size_t i = 0; i < num_funcs; i++) {
		if (strcmp(builtin_funcs[i].func_name, func_name) == 0) {
			builtin_funcs[i].func = func;
			builtin_funcs[i].is_defined = value;
			return true;
		}
	}
	return false;
}

static LLVMValueRef codegen_number(const struct lex_token *token) {
	return LLVMConstInt(LLVMInt32Type(), token->literal.number, 0);
}

static LLVMValueRef codegen_expression(const struct ast_node *node, const struct strmap *var_map);

static LLVMValueRef codegen_func_call(const struct ast_node *node, const struct strmap *var_map) {
	const char *func_name = node->value.children.l[0].value.token.str;

	bool is_defined;
	LLVMTypeRef func_type;
	LLVMValueRef func;
	bool is_builtin = get_builtin_func(func_name, &is_defined, &func, &func_type);

	if (!is_builtin) {
		fprintf(stderr, "NON BUILTIN FUNCTION!\n");
		return NULL;
	}

	if (!is_defined) {
		func = LLVMAddFunction(module, func_name, func_type);
		set_builtin_func(func_name, true, func);
	}
		
	if (strcmp(func_name, "getchar") != 0)
		return NULL;

	return LLVMBuildIntCast2(
		builder,
		LLVMBuildCall2(
			builder,
			LLVMFunctionType(LLVMInt8Type(), NULL, 0, 0),
			func,
			NULL,
			0,
			"getchartmp"
		),
		LLVMInt32Type(),
		true,
		"getcharcasttmp"
	);
}

static LLVMValueRef codegen_factor(const struct ast_node *node, const struct strmap *var_map) {
	const struct ast_node *child = &node->value.children.l[0];

	if (child->type == AST_EXPR)
		return codegen_expression(child, var_map);

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
		LLVMValueRef value = codegen_func_call(child, var_map);
		if (value != NULL)
			return value;
	}

	fprintf(stderr, "ERROR!\n");
	exit(1);
}

static LLVMValueRef codegen_term(const struct ast_node *node, const struct strmap *var_map) {
	if (node->type != AST_TERM) {
		fprintf(stderr, "ERROR!\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;
	LLVMValueRef lhs = codegen_factor(&list->l[0], var_map);

	size_t i;
	for (i = 1; i < list->size - 1; i += 2) {
		if (list->l[i].type != AST_LEAF) {
			fprintf(stderr, "ERROR! (1)");
			exit(1);
		}

		LLVMValueRef rhs = codegen_factor(&list->l[i + 1], var_map);

		switch (list->l[i].value.token.type) {
			case LEX_STAR:
				lhs = LLVMBuildMul(builder, lhs, rhs, "multmp");
				break;
			case LEX_SLASH:
				lhs = LLVMBuildSDiv(builder, lhs, rhs, "divtmp");
				break;
			case LEX_PERCENT:
				lhs = LLVMBuildSRem(builder, lhs, rhs, "modtmp");
				break;
			default:
				fprintf(stderr, "ERROR! (2)");
				exit(1);
		}
	}

	if (i != list->size) {
		fprintf(stderr, "ERROR! (2)");
		exit(1);
	}

	return lhs;
}

static LLVMValueRef codegen_expr_no_comp(const struct ast_node *node, const struct strmap *var_map) {
	if (node->type != AST_EXPR_NO_COMP) {
		fprintf(stderr, "ERROR!\n");
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

	LLVMValueRef lhs = codegen_term(&list->l[i], var_map);
	if (first_is_negative)
		lhs = LLVMBuildMul(builder, lhs, LLVMConstInt(LLVMInt32Type(), -1, 0), "negtmp");

	for (i = i + 1; i < list->size - 1; i += 2) {
		if (list->l[i].type != AST_LEAF) {
			fprintf(stderr, "ERROR! (3)");
			exit(1);
		}

		LLVMValueRef rhs = codegen_term(&list->l[i + 1], var_map);

		if (list->l[i].value.token.type == LEX_PLUS)
			lhs = LLVMBuildAdd(builder, lhs, rhs, "addtmp");
		else if (list->l[i].value.token.type == LEX_MINUS)
			lhs = LLVMBuildSub(builder, lhs, rhs, "subtmp");
	}

	if (i != list->size) {
		fprintf(stderr, "ERROR! (4)");
		exit(1);
	}

	return lhs;
}

static LLVMValueRef codegen_expression(const struct ast_node *node, const struct strmap *var_map) {
	if (node->type != AST_EXPR) {
		fprintf(stderr, "ERROR!\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;

	// not comparison, only child is expr_no_comp
	if (list->size == 1)
		return codegen_expr_no_comp(&list->l[0], var_map);

	if (list->size == 3) {
		LLVMValueRef lhs = codegen_expr_no_comp(&list->l[0], var_map);
		enum lex_token_type comp_type = list->l[1].value.token.type;
		LLVMValueRef rhs = codegen_expr_no_comp(&list->l[2], var_map);

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
				fprintf(stderr, "ERROR! (5)");
				exit(1);
		}

		LLVMValueRef bool_value = LLVMBuildICmp(builder, comp_pred, lhs, rhs, "cmptmp");
		return LLVMBuildIntCast2(builder, bool_value, LLVMInt32Type(), false, "cmptmp2");
	}

	fprintf(stderr, "ERROR! (6)");
	exit(1);
}

static void codegen_assignment(const struct ast_node *node, struct strmap *var_map) {
	if (node->type != AST_ASSIGN) {
		fprintf(stderr, "ERROR!\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;

	if (list->size != 2 || list->l[0].type != AST_LEAF) {
		fprintf(stderr, "ERROR!\n");
		exit(1);
	}

	const struct lex_token *ident = &list->l[0].value.token;
	LLVMValueRef rhs = codegen_expression(&list->l[1], var_map);
	strmap_set(var_map, ident->str, &rhs, sizeof(rhs));
}

static void codegen_return(const struct ast_node *node, struct strmap *var_map) {
	if (node->type != AST_RETURN) {
		fprintf(stderr, "ERROR!\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;

	if (list->size != 1 || list->l[0].type != AST_EXPR) {
		fprintf(stderr, "ERROR!\n");
		exit(1);
	}

	LLVMValueRef value = codegen_expression(&list->l[0], var_map);
	LLVMBuildRet(builder, value);
}

static void codegen_statement(const struct ast_node *node, struct strmap *var_map) {
	if (node->type != AST_STMT) {
		fprintf(stderr, "ERROR!\n");
		exit(1);
	}

	const struct ast_node *child = &node->value.children.l[0];
	switch (child->type) {
		case AST_ASSIGN:
			codegen_assignment(child, var_map);
			break;	
		case AST_RETURN:
			codegen_return(child, var_map);
			break;
		default:
			fprintf(stderr, "ERROR!\n");
			exit(1);
	}
}

static void codegen_stmt_list(const struct ast_node *node, struct strmap *var_map) {
	if (node->type != AST_STMT_LIST) {
		fprintf(stderr, "ERROR!\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;
	for (size_t i = 0; i < list->size; i++)
		codegen_statement(&list->l[i], var_map);
}

bool codegen(const char *name, const struct ast_node *root) {
    module = LLVMModuleCreateWithName(name);

	// start
	// LLVMTypeRef getchar_param_types[] = { };
	// LLVMTypeRef getchar_ret_type = LLVMFunctionType(LLVMInt8Type(), getchar_param_types, 0, 0);
	// LLVMValueRef getchar_func = LLVMAddFunction(mod, "getchar", getchar_ret_type);
	// end

    LLVMTypeRef param_types[] = { };
    LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(module, "main", ret_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry");

    builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);

	// start
	// LLVMValueRef params[] = { LLVMConstInt(LLVMInt32Type(), 0, 0) };
	// LLVMValueRef getchar_val = LLVMBuildIntCast2(
	// 	builder,
	// 	LLVMBuildCall2(
	// 		builder,
	// 		LLVMFunctionType(LLVMInt8Type(), NULL, 0, 0),
	// 		getchar_func,
	// 		params,
	// 		0,
	// 		"getchartmp"
	// 	),
	// 	LLVMInt32Type(),
	// 	true,
	// 	"getcharcasttmp"
	// );
	//
	// LLVMBuildRet(builder, getchar_val);
	// end

	struct strmap var_map = strmap_new();
	codegen_stmt_list(&root->value.children.l[0], &var_map);

	char *error = NULL;
	LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
	LLVMDisposeMessage(error);

	size_t len = strlen(name);
	len += 4; // ".bc" and a null terminator
	char *bitcode_filename = malloc(len * sizeof(char));
	strcpy(bitcode_filename, name);
	strcat(bitcode_filename, ".bc");

    // Write out bitcode to file
	if (LLVMWriteBitcodeToFile(module, bitcode_filename) != 0)
		fprintf(stderr, "error writing bitcode to file, skipping\n");

	free(bitcode_filename);
	strmap_free(&var_map);

    LLVMDisposeBuilder(builder);

	return true;
}


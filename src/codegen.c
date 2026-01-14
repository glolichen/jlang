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

struct function_info {
	LLVMValueRef func;
	LLVMTypeRef type;
	bool is_builtin;

	// if builtin, whether is declared in bitcode
	// if not builtin, whether is defined
	bool is_defined_or_declared;
};

// static struct {
// 	const char *func_name;
// 	LLVMValueRef func;
// 	bool is_defined;
// } builtin_funcs[] = {
// 	{ "getchar", NULL, 0 },
// };
//
// // -1 = not builtin func, 0 = not defined, 1 = defined
// static bool get_builtin_func(const char *func_name, bool *defined, LLVMValueRef *func, LLVMTypeRef *type) {
// 	size_t num_funcs = sizeof(builtin_funcs) / sizeof(builtin_funcs[0]);
// 	for (size_t i = 0; i < num_funcs; i++) {
// 		if (strcmp(builtin_funcs[i].func_name, func_name) == 0) {
// 			*defined = builtin_funcs[i].is_defined;
// 			*func = builtin_funcs[i].func;
// 			*type = LLVMFunctionType(LLVMInt8Type(), NULL, 0, 0);
// 			return true;
// 		}
// 	}
// 	return false;
// }
// static bool set_builtin_func(const char *func_name, bool value, LLVMValueRef func) {
// 	size_t num_funcs = sizeof(builtin_funcs) / sizeof(builtin_funcs[0]);
// 	for (size_t i = 0; i < num_funcs; i++) {
// 		if (strcmp(builtin_funcs[i].func_name, func_name) == 0) {
// 			builtin_funcs[i].func = func;
// 			builtin_funcs[i].is_defined = value;
// 			return true;
// 		}
// 	}
// 	return false;
// }

static void populate_builtin_funcs(struct strmap *func_map) {
	struct function_info getchar_info = {
		.func = NULL,
		.type = LLVMFunctionType(LLVMInt8Type(), NULL, 0, 0),
		.is_builtin = true,
		.is_defined_or_declared = false
	};
	strmap_set(func_map, "getchar", &getchar_info, sizeof(getchar_info));
}

static LLVMValueRef codegen_number(const struct lex_token *token) {
	return LLVMConstInt(LLVMInt32Type(), token->literal.number, 0);
}

static LLVMValueRef codegen_expression(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
);

static LLVMValueRef codegen_func_call(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	(void) var_map;

	const char *func_name = node->value.children.l[0].value.token.str;
	struct function_info *func_info = strmap_get(func_map, func_name);

	if (func_info == NULL) {
		fprintf(stderr, "function not defined!\n");
		return NULL;
	}

	if (!func_info->is_builtin) {
		fprintf(stderr, "TODO: non builtin function\n");
		return NULL;
	}

	LLVMValueRef func_ref;
	if ((func_ref = func_info->func) == NULL) {
		func_info->func = func_ref = LLVMAddFunction(mod, func_name, func_info->type);
		strmap_set(func_map, func_name, func_info, sizeof(*func_info));
	}
		
	if (strcmp(func_name, "getchar") != 0)
		return NULL;

	return LLVMBuildIntCast2(
		build,
		LLVMBuildCall2(
			build, LLVMFunctionType(LLVMInt8Type(), NULL, 0, 0),
			func_ref, NULL, 0, "getchartmp"
		),
		LLVMInt32Type(),
		true,
		"getcharcasttmp"
	);
}

static LLVMValueRef codegen_factor(
	LLVMModuleRef module, LLVMBuilderRef builder,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	const struct ast_node *child = &node->value.children.l[0];

	if (child->type == AST_EXPR)
		return codegen_expression(module, builder, child, var_map, func_map);

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
		LLVMValueRef value = codegen_func_call(module, builder, child, var_map, func_map);
		if (value != NULL)
			return value;
	}

	fprintf(stderr, "ERROR!\n");
	exit(1);
}

static LLVMValueRef codegen_term(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_TERM) {
		fprintf(stderr, "ERROR!\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;
	LLVMValueRef lhs = codegen_factor(mod, build, &list->l[0], var_map, func_map);

	size_t i;
	for (i = 1; i < list->size - 1; i += 2) {
		if (list->l[i].type != AST_LEAF) {
			fprintf(stderr, "ERROR! (1)");
			exit(1);
		}

		LLVMValueRef rhs = codegen_factor(mod, build, &list->l[i + 1], var_map, func_map);

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

static LLVMValueRef codegen_expr_no_comp(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
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

	LLVMValueRef lhs = codegen_term(mod, build, &list->l[i], var_map, func_map);
	if (first_is_negative)
		lhs = LLVMBuildMul(build, lhs, LLVMConstInt(LLVMInt32Type(), -1, 0), "negtmp");

	for (i = i + 1; i < list->size - 1; i += 2) {
		if (list->l[i].type != AST_LEAF) {
			fprintf(stderr, "ERROR! (3)");
			exit(1);
		}

		LLVMValueRef rhs = codegen_term(mod, build, &list->l[i + 1], var_map, func_map);

		if (list->l[i].value.token.type == LEX_PLUS)
			lhs = LLVMBuildAdd(build, lhs, rhs, "addtmp");
		else if (list->l[i].value.token.type == LEX_MINUS)
			lhs = LLVMBuildSub(build, lhs, rhs, "subtmp");
	}

	if (i != list->size) {
		fprintf(stderr, "ERROR! (4)");
		exit(1);
	}

	return lhs;
}

static LLVMValueRef codegen_expression(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_EXPR) {
		fprintf(stderr, "ERROR!\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;

	// not comparison, only child is expr_no_comp
	if (list->size == 1)
		return codegen_expr_no_comp(mod, build, &list->l[0], var_map, func_map);

	if (list->size == 3) {
		LLVMValueRef lhs = codegen_expr_no_comp(mod, build, &list->l[0], var_map, func_map);
		enum lex_token_type comp_type = list->l[1].value.token.type;
		LLVMValueRef rhs = codegen_expr_no_comp(mod, build, &list->l[2], var_map, func_map);

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

		LLVMValueRef bool_value = LLVMBuildICmp(build, comp_pred, lhs, rhs, "cmptmp");
		return LLVMBuildIntCast2(build, bool_value, LLVMInt32Type(), false, "cmptmp2");
	}

	fprintf(stderr, "ERROR! (6)");
	exit(1);
}

static void codegen_assignment(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
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
	LLVMValueRef rhs = codegen_expression(mod, build, &list->l[1], var_map, func_map);
	strmap_set(var_map, ident->str, &rhs, sizeof(rhs));
}

static void codegen_return(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_RETURN) {
		fprintf(stderr, "ERROR!\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;

	if (list->size != 1 || list->l[0].type != AST_EXPR) {
		fprintf(stderr, "ERROR!\n");
		exit(1);
	}

	LLVMValueRef value = codegen_expression(mod, build, &list->l[0], var_map, func_map);
	LLVMBuildRet(build, value);
}

static void codegen_statement(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_STMT) {
		fprintf(stderr, "ERROR!\n");
		exit(1);
	}

	const struct ast_node *child = &node->value.children.l[0];
	switch (child->type) {
		case AST_ASSIGN:
			codegen_assignment(mod, build, child, var_map, func_map);
			break;	
		case AST_RETURN:
			codegen_return(mod, build, child, var_map, func_map);
			break;
		default:
			fprintf(stderr, "ERROR!\n");
			exit(1);
	}
}

static void codegen_stmt_list(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_STMT_LIST) {
		fprintf(stderr, "ERROR!\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;
	for (size_t i = 0; i < list->size; i++)
		codegen_statement(mod, build, &list->l[i], var_map, func_map);
}

bool codegen(const char *name, const struct ast_node *root) {
    LLVMModuleRef module = LLVMModuleCreateWithName(name);

	// start
	// LLVMTypeRef getchar_param_types[] = { };
	// LLVMTypeRef getchar_ret_type = LLVMFunctionType(LLVMInt8Type(), getchar_param_types, 0, 0);
	// LLVMValueRef getchar_func = LLVMAddFunction(mod, "getchar", getchar_ret_type);
	// end

    LLVMTypeRef param_types[] = { };
    LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(module, "main", ret_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry");

    LLVMBuilderRef builder = LLVMCreateBuilder();
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

	struct strmap var_map = strmap_new(), func_map = strmap_new();
	populate_builtin_funcs(&func_map);
	codegen_stmt_list(module, builder, &root->value.children.l[0], &var_map, &func_map);

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
	strmap_free(&func_map);

    LLVMDisposeBuilder(builder);

	return true;
}


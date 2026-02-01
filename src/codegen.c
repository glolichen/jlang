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

#define NUM_PARAMS(params) (sizeof(params) / sizeof(params[0]))

struct function_info {
	// if null, then not declared yet
	LLVMValueRef func;

	LLVMTypeRef type;
	bool is_builtin;

	// if not builtin, whether is defined
	bool is_defined;
};

static void populate_builtin_funcs(struct strmap *func_map) {
	struct function_info getchar_info = {
		.func = NULL,
		.type = LLVMFunctionType(LLVMInt8Type(), NULL, 0, 0),
		.is_builtin = true,
		.is_defined = false
	};
	strmap_set(func_map, "getchar", &getchar_info, sizeof(getchar_info));

	LLVMTypeRef putchar_params[] = { LLVMInt32Type() };
	struct function_info putchar_info = {
		.func = NULL,
		.type = LLVMFunctionType(LLVMVoidType(), putchar_params, NUM_PARAMS(putchar_params), 0),
		.is_builtin = true,
		.is_defined = false
	};
	strmap_set(func_map, "putchar", &putchar_info, sizeof(putchar_info));
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
		fprintf(stderr, "function %s not defined!\n", func_name);
		exit(1);
	}

	if (!func_info->is_builtin) {
		// TODO: add non builtin functions
		fprintf(stderr, "TODO: non builtin function\n");
		exit(1);
	}

	if (func_info->func == NULL) {
		// this will modify the value in the map as well
		func_info->func = LLVMAddFunction(mod, func_name, func_info->type);
	}
		
	struct ast_node_list *ast_params = &node->value.children.l[1].value.children;
	size_t ast_param_num = ast_params->size;

	if (ast_param_num != LLVMCountParamTypes(func_info->type)) {
		fprintf(stderr, "invalid number of parameters\n");
		exit(1);
	}

	if (strcmp(func_name, "getchar") == 0) {
		return LLVMBuildIntCast2(
			build,
			LLVMBuildCall2(build, func_info->type, func_info->func, NULL, 0, "getchartmp"),
			LLVMInt32Type(),
			true,
			"getcharcasttmp"
		);
	}

	LLVMValueRef *params;
	if (ast_param_num == 0)
		params = NULL;
	else {
		params = malloc(ast_param_num * sizeof(LLVMValueRef));
		for (size_t i = 0; i < ast_param_num; i++)
			params[i] = codegen_expression(mod, build, &ast_params->l[i], var_map, func_map);
	}

	LLVMValueRef out = LLVMBuildCall2(build, func_info->type, func_info->func, params, ast_param_num, "");
	free(params);

	return out;
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

	fprintf(stderr, "ERROR! (1)\n");
	exit(1);
}

static LLVMValueRef codegen_term(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_TERM) {
		fprintf(stderr, "ERROR! (2)\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;
	LLVMValueRef lhs = codegen_factor(mod, build, &list->l[0], var_map, func_map);

	size_t i;
	for (i = 1; i < list->size - 1; i += 2) {
		if (list->l[i].type != AST_LEAF) {
			fprintf(stderr, "ERROR! (3)");
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

static LLVMValueRef codegen_expr_no_comp(
	LLVMModuleRef mod, LLVMBuilderRef build,
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

	LLVMValueRef lhs = codegen_term(mod, build, &list->l[i], var_map, func_map);
	if (first_is_negative)
		lhs = LLVMBuildMul(build, lhs, LLVMConstInt(LLVMInt32Type(), -1, 0), "negtmp");

	for (i = i + 1; i < list->size - 1; i += 2) {
		if (list->l[i].type != AST_LEAF) {
			fprintf(stderr, "ERROR! (7)");
			exit(1);
		}

		LLVMValueRef rhs = codegen_term(mod, build, &list->l[i + 1], var_map, func_map);

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

static LLVMValueRef codegen_expression(
	LLVMModuleRef mod, LLVMBuilderRef build,
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
				fprintf(stderr, "ERROR! (10)");
				exit(1);
		}

		LLVMValueRef bool_value = LLVMBuildICmp(build, comp_pred, lhs, rhs, "cmptmp");
		return LLVMBuildIntCast2(build, bool_value, LLVMInt32Type(), false, "cmptmp2");
	}

	fprintf(stderr, "ERROR! (11)");
	exit(1);
}

static void codegen_assignment(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_ASSIGN) {
		fprintf(stderr, "ERROR! (12)\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;

	if (list->size != 2 || list->l[0].type != AST_LEAF) {
		fprintf(stderr, "ERROR! (13)\n");
		exit(1);
	}

	const struct lex_token *ident = &list->l[0].value.token;
	LLVMValueRef rhs = codegen_expression(mod, build, &list->l[1], var_map, func_map);
	strmap_set(var_map, ident->str, &rhs, sizeof(LLVMValueRef));
}

static void codegen_return(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_RETURN) {
		fprintf(stderr, "ERROR! (14)\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;

	if (list->size != 1 || list->l[0].type != AST_EXPR) {
		fprintf(stderr, "ERROR! (15)\n");
		exit(1);
	}

	LLVMValueRef value = codegen_expression(mod, build, &list->l[0], var_map, func_map);
	LLVMBuildRet(build, value);
}

static void codegen_stmt_list(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
);

// will modify var_map using phi nodes
void codegen_conditional(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	LLVMValueRef condition = codegen_expression(
		mod, build,
		&node->value.children.l[0],
		var_map, func_map
	);
	if (condition == NULL) {
		fprintf(stderr, "ERROR! (19)\n");
		exit(1);
	}

	condition = LLVMBuildICmp(
		build, LLVMIntNE, condition,
		LLVMConstInt(LLVMInt32Type(), 0, 0),
		"ifcmptmp"
	);

	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(build));

	LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func, "then");
	LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(func, "else");
	LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(func, "ifcont");

	LLVMBuildCondBr(build, condition, then_block, else_block);



	// generate then block and add merge block to terminate it
	LLVMPositionBuilderAtEnd(build, then_block);
	struct strmap var_map_then = strmap_copy(var_map);
	codegen_stmt_list(mod, build, &node->value.children.l[1], &var_map_then, func_map);

	LLVMBuildBr(build, merge_block);
	then_block = LLVMGetInsertBlock(build);



	// generate else block and add merge block to terminate it
	LLVMPositionBuilderAtEnd(build, else_block);
	struct strmap var_map_else = strmap_copy(var_map);
	codegen_stmt_list(mod, build, &node->value.children.l[2], &var_map_else, func_map);

	LLVMBuildBr(build, merge_block);
	else_block = LLVMGetInsertBlock(build);



	// deal with merge blocks and add phi nodes
	LLVMPositionBuilderAtEnd(build, merge_block);

	// iterate through ALREADY DEFINED variables and assign with phi nodes
	// but only do this if they have been modified by either block
	for (uint64_t i = 0; i < var_map->bucket_count; i++) {
		struct strmap_list_node *cur = var_map->list[i];
		while (cur != NULL) {
			LLVMValueRef value_cur = *(LLVMValueRef *) cur->value;
			LLVMValueRef value_then = *(LLVMValueRef *) strmap_get(&var_map_then, cur->str);
			LLVMValueRef value_else = *(LLVMValueRef *) strmap_get(&var_map_else, cur->str);

			// printf("var %s: old %p, then %p, else %p\n", cur->str, value_cur, value_then, value_else);

			// if conditional does not affect value, no need for phi
			if (value_cur == value_then && value_cur == value_else) {
				cur = cur->next;
				continue;
			}

			// printf("phi\n");

			LLVMValueRef phi = LLVMBuildPhi(build, LLVMInt32Type(), "phitmp");
			LLVMAddIncoming(phi, &value_then, &then_block, 1);
			LLVMAddIncoming(phi, &value_else, &else_block, 1);

			// printf("the phi value is %p\n", phi);

			// need to strmap_set to copy the phi value into the var_map
			// the var_map copies for then and else are free
			strmap_set(var_map, cur->str, &phi, sizeof(LLVMValueRef));

			cur = cur->next;
		}
	}
	
	strmap_free(&var_map_then);
	strmap_free(&var_map_else);
}

void codegen_for_loop(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	// NOTE: adding the initial value both var_map and var_map_loop
	// this is so that we can use a phi when setting the variable in the body of the loop
	// this can be removed from the var_map at the end (if variable not declared/defined before)
	// FIXME: have this removed, if it's not already declared
	if (node->value.children.l[0].value.children.size != 0)
		codegen_assignment(mod, build, &node->value.children.l[0], var_map, func_map);
	struct strmap var_map_loop = strmap_copy(var_map);

	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(build));

	LLVMBasicBlockRef before_block = LLVMGetInsertBlock(build);
	LLVMBasicBlockRef loop_block = LLVMAppendBasicBlock(func, "loop");

	LLVMBuildBr(build, loop_block);
	LLVMPositionBuilderAtEnd(build, loop_block);

	// hack
	// LLVMValueRef phi = LLVMBuildPhi(build, LLVMInt32Type(), "phitmp");
	// LLVMValueRef value_before = *(LLVMValueRef *) strmap_get(var_map, "i");
	// LLVMAddIncoming(phi, &value_before, &before_block, 1);
	// strmap_set(&var_map_loop, "i", &phi, sizeof(LLVMValueRef));

	// FIXME: need a phi on ALL variables, not just i like it is now
	// because they might have been modified by the loop
	// need to select the correct one based whether the loop has been executed before

	struct strmap loop_phi_nodes = strmap_new();

	for (uint64_t i = 0; i < var_map->bucket_count; i++) {
		// linked list current node for var_map before loop (in "var_map" variable)
		struct strmap_list_node *cur_before = var_map->list[i];
		while (cur_before != NULL) {
			LLVMValueRef phi = LLVMBuildPhi(build, LLVMInt32Type(), "phitmp");
			LLVMValueRef value_before = *(LLVMValueRef *) cur_before->value;
			LLVMAddIncoming(phi, &value_before, &before_block, 1);
			strmap_set(&loop_phi_nodes, cur_before->str, &phi, sizeof(LLVMValueRef));
			strmap_set(&var_map_loop, cur_before->str, &phi, sizeof(LLVMValueRef));

			cur_before = cur_before->next;
		}
	}

	codegen_stmt_list(mod, build, &node->value.children.l[3], &var_map_loop, func_map); 

	codegen_assignment(mod, build, &node->value.children.l[2], &var_map_loop, func_map);

	LLVMValueRef end_condition = codegen_expression(
		mod, build,
		&node->value.children.l[1],
		&var_map_loop, func_map
	);
	if (end_condition == NULL) {
		fprintf(stderr, "ERROR! (67)\n");
		exit(1);
	}
	end_condition = LLVMBuildICmp(
		build, LLVMIntNE, end_condition,
		LLVMConstInt(LLVMInt32Type(), 0, 0),
		"forcmptmp"
	);

	LLVMBasicBlockRef loop_block_end = LLVMGetInsertBlock(build);
	LLVMBasicBlockRef after_block = LLVMAppendBasicBlock(func, "forcont");

	LLVMBuildCondBr(build, end_condition, loop_block, after_block);

	LLVMPositionBuilderAtEnd(build, after_block);

	for (uint64_t i = 0; i < loop_phi_nodes.bucket_count; i++) {
		// linked list current node for var_map before loop (in "var_map" variable)
		struct strmap_list_node *cur_phi = loop_phi_nodes.list[i];
		while (cur_phi != NULL) {
			LLVMValueRef phi = *(LLVMValueRef *) cur_phi->value;
			LLVMValueRef value_loop = *(LLVMValueRef *) strmap_get(&var_map_loop, cur_phi->str);
			LLVMAddIncoming(phi, &value_loop, &loop_block_end, 1);

			cur_phi = cur_phi->next;
		}
	}
	// LLVMValueRef value_loop = *(LLVMValueRef *) strmap_get(&var_map_loop, "i");
	// LLVMAddIncoming(phi, &value_loop, &loop_block_end, 1);




	// printf("%p\n", *(LLVMValueRef *) strmap_get(&var_map_loop, "i"));

	// strmap_set(var_map, "a", strmap_get(&var_map_loop, "i"), sizeof(LLVMValueRef));




	// deal with merge blocks and add phi nodes
	LLVMPositionBuilderAtEnd(build, after_block);

	// iterate through ALREADY DEFINED variables and assign with phi nodes
	// but only do this if they have been modified by either block
	for (uint64_t i = 0; i < var_map->bucket_count; i++) {
		struct strmap_list_node *cur = var_map->list[i];
		while (cur != NULL) {
			LLVMValueRef value_cur = *(LLVMValueRef *) cur->value;
			LLVMValueRef value_loop = *(LLVMValueRef *) strmap_get(&var_map_loop, cur->str);

			if (value_cur != value_loop)
				strmap_set(var_map, cur->str, &value_loop, sizeof(LLVMValueRef));

			cur = cur->next;
		}
	}
	
	strmap_free(&var_map_loop);
	strmap_free(&loop_phi_nodes);
}

static void codegen_statement(
	LLVMModuleRef mod, LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->type != AST_STMT) {
		fprintf(stderr, "ERROR! (16)\n");
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
		case AST_FUNC_CALL:
			codegen_func_call(mod, build, child, var_map, func_map);
			break;
		case AST_CONDITIONAL:
			codegen_conditional(mod, build, child, var_map, func_map);
			break;
		case AST_FOR:
			codegen_for_loop(mod, build, child, var_map, func_map);
			break;
		default:
			fprintf(stderr, "ERROR! (17)\n");
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
		fprintf(stderr, "ERROR! (18)\n");
		exit(1);
	}

	const struct ast_node_list *list = &node->value.children;
	for (size_t i = 0; i < list->size; i++)
		codegen_statement(mod, build, &list->l[i], var_map, func_map);
}

// void codegen_test(
// 	LLVMModuleRef mod, LLVMBuilderRef build
// ) {
// 	LLVMValueRef val_a = LLVMConstInt(LLVMInt32Type(), 0, 0);
// 	LLVMValueRef val_b = LLVMConstInt(LLVMInt32Type(), 0, 0);
//
// 	LLVMTypeRef getchar_type = LLVMFunctionType(LLVMInt8Type(), NULL, 0, 0);
// 	struct function_info getchar_info = {
// 		.func = NULL,
// 		.type = getchar_type,
// 		.is_builtin = true,
// 		.is_defined = false
// 	};
// 	LLVMValueRef getchar_func = LLVMAddFunction(mod, "getchar", getchar_type);
// 	LLVMValueRef val_input = LLVMBuildIntCast2(
// 		build,
// 		LLVMBuildCall2(
// 			build,
// 			getchar_type, getchar_func,
// 			NULL, 0, "getchartmp"
// 		),
// 		LLVMInt32Type(),
// 		true,
// 		"getcharcasttmp"
// 	);
//
// 	LLVMValueRef condition = LLVMBuildICmp(
// 		build, LLVMIntEQ,
// 		val_input,
// 		LLVMConstInt(LLVMInt32Type(), 48, 0),
// 		"ifcmptmp"
// 	);
//
// 	// Retrieve function.
// 	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(build));
//
// 	// Generate true/false expr and merge.
// 	LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func, "then");
// 	LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(func, "else");
// 	LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(func, "ifcont");
//
// 	LLVMBuildCondBr(build, condition, then_block, else_block);
//
// 	// Generate 'then' block.
// 	LLVMPositionBuilderAtEnd(build, then_block);
// 	LLVMValueRef then_value = LLVMConstInt(LLVMInt32Type(), 10, 0);
//
// 	LLVMBuildBr(build, merge_block);
// 	then_block = LLVMGetInsertBlock(build);
//
// 	LLVMPositionBuilderAtEnd(build, else_block);
// 	LLVMValueRef else_value = LLVMConstInt(LLVMInt32Type(), 1, 0);
//
// 	LLVMBuildBr(build, merge_block);
// 	else_block = LLVMGetInsertBlock(build);
//
// 	LLVMPositionBuilderAtEnd(build, merge_block);
// 	LLVMValueRef phi = LLVMBuildPhi(build, LLVMInt32Type(), "phi");
// 	LLVMAddIncoming(phi, &then_value, &then_block, 1);
// 	LLVMAddIncoming(phi, &else_value, &else_block, 1);
//
//
// 	LLVMValueRef addtmp = LLVMBuildAdd(build, val_a, phi, "addtmp");
// 	LLVMBuildRet(build, addtmp);
// }

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

	// codegen_test(module, builder);

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


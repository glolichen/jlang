#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen/function.h"
#include "codegen/codegen.h"
#include "codegen/expression.h"
#include "codegen/statement.h"
#include "codegen/variable.h"
#include "error.h"
#include "utils/strmap.h"
#include "types.h"
#include "ast.h"
#include "lex.h"

#define NUM_PARAMS(params) (sizeof(params) / sizeof(params[0]))

struct function_info {
	// if null, then not declared yet
	LLVMValueRef func;

	LLVMTypeRef llvm_type;

	size_t num_params;
	const struct type *const params;
	struct type return_type;

	bool is_builtin;
};

static LLVMTypeRef get_llvm_function_type(
	LLVMBuilderRef build,
	size_t num_params,
	const struct type *const params,
	struct type return_type
) {
	LLVMTypeRef *llvm_param_types = malloc(num_params * sizeof(LLVMTypeRef));
	for (size_t i = 0; i < num_params; i++)
		llvm_param_types[i] = type_to_llvm_type(build, params[i], false);

	LLVMTypeRef type = LLVMFunctionType(
		type_to_llvm_type(build, return_type, true),
		llvm_param_types, num_params, 0
	);

	free(llvm_param_types);
	return type;
}

void codegen_func_init(LLVMBuilderRef build, struct strmap *func_map) {
	size_t getchar_num_params = 0;
	struct type *getchar_params = malloc(
		getchar_num_params * sizeof(struct type)
	);
	struct type getchar_return_type = { .kind = TYPE_I32 };
	strmap_set(func_map, "getchar", & (struct function_info) {
		.func = NULL,
		.llvm_type = get_llvm_function_type(
			build,
			getchar_num_params,
			getchar_params,
			getchar_return_type
		),
		.num_params = getchar_num_params,
		.params = getchar_params,
		.return_type = getchar_return_type,
		.is_builtin = true
	}, sizeof(struct function_info));


	size_t putchar_num_params = 1;
	struct type *putchar_params = malloc(
		putchar_num_params * sizeof(struct type)
	);
	putchar_params[0] = (struct type) { .kind = TYPE_I32 };
	struct type putchar_return_type = { .kind = TYPE_VOID };
	strmap_set(func_map, "putchar", & (struct function_info) {
		.func = NULL,
		.llvm_type = get_llvm_function_type(
			build,
			putchar_num_params,
			putchar_params,
			putchar_return_type
		),
		.num_params = putchar_num_params,
		.params = putchar_params,
		.return_type = putchar_return_type,
		.is_builtin = true
	}, sizeof(struct function_info));
}

void codegen_func_free(struct strmap *func_map) {
	for (uint64_t i = 0; i < func_map->bucket_count; i++) {
		struct strmap_list_node *node = func_map->list[i];
		while (node != NULL) {
			struct function_info func = *(struct function_info *) node->value;

			if (func.params) {
				for (size_t j = 0; j < func.num_params; j++)
					type_free((struct type *) &func.params[j]);
				free((void *) func.params);
			}

			node = node->next;
		}
	}
}

LLVMValueRef codegen_func_call(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	const char *func_name = node->value.children.l[0].value.token.str;

	if (
		strcmp(func_name, "toi8") == 0 ||
		strcmp(func_name, "toi16") == 0 ||
		strcmp(func_name, "toi32") == 0 ||
		strcmp(func_name, "toi64") == 0
	) {
		return types_convert_literal(build, node, var_map, func_map);
	}

	struct function_info *func_info = strmap_get(func_map, func_name);

	if (func_info == NULL) {
		fprintf(
			stderr,
			"[ERROR] line %zu: function %s not defined\n",
			node->line, func_name
		);
		exit(1);
	}

	if (func_info->func == NULL) {
		if (!func_info->is_builtin)
			ERROR_COMPILER();

		// this will modify the value in the map as well
		LLVMModuleRef module = codegen_get_current_module();
		func_info->func = LLVMAddFunction(module, func_name, func_info->llvm_type);
	}
		
	struct ast_node_list *ast_params = &node->value.children.l[1].value.children;
	size_t ast_num_params = ast_params->size;

	// extract function parameter types for checking
	size_t num_params = func_info->num_params;

	if (ast_num_params != num_params) {
		fprintf(
			stderr,
			"[ERROR] line %zu: invalid number of parameters: expected %zu, got %zu\n",
			node->line, num_params, ast_num_params
		);
		exit(1);
	}

	const struct type *const param_types = func_info->params;

	LLVMValueRef *params;
	if (ast_num_params == 0)
		params = NULL;
	else {
		params = malloc(ast_num_params * sizeof(LLVMValueRef));
		for (size_t i = 0; i < ast_num_params; i++) {
			params[i] = codegen_expression(build, &ast_params->l[i], var_map, func_map);

			// check types
			struct type got_type = ast_params->l[i].value_type;
			if (!type_eq(got_type, param_types[i])) {
				fprintf(
					stderr,
					"[ERROR] line %zu: type mismatch: expected %s, got %s\n",
					node->line,
					type_to_str(param_types[i]),
					type_to_str(got_type)
				);
				exit(1);
			}
		}
	}

	LLVMValueRef out = LLVMBuildCall2(
		build,
		func_info->llvm_type,
		func_info->func, params,
		ast_num_params,
		"call"
	);

	if (params != NULL)
		free(params);

	node->value_type = func_info->return_type;

	return out;
}

struct type func_return_type = { 0 };
LLVMValueRef func_ref = NULL;

void codegen_func_definition(
	LLVMModuleRef module,
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	struct ast_node_list children = node->value.children;
	if (children.size != 4)
		ERROR_COMPILER();

	struct strmap var_map_copy = strmap_copy(var_map);

	func_return_type = type_from_ast_node(&children.l[0], true);

	const char *func_name = children.l[1].value.token.str;

	struct ast_node_list node_params = children.l[2].value.children;
	size_t num_params = node_params.size / 2;
	struct type *param_types = malloc(num_params * sizeof(struct type));
	for (size_t i = 0; i < num_params; i++) {
		param_types[i] = type_from_ast_node(
			&node_params.l[i * 2], false
		);
	}

	LLVMTypeRef func_type = get_llvm_function_type(
		build, num_params, param_types, func_return_type
	);

	func_ref = LLVMAddFunction(module, func_name, func_type);

	LLVMContextRef llvm_ctx = LLVMGetBuilderContext(build);
	LLVMBasicBlockRef entry_block = LLVMAppendBasicBlockInContext(
		llvm_ctx, func_ref, "entry"
	);
	LLVMPositionBuilderAtEnd(build, entry_block);
	
	for (size_t i = 0; i < num_params; i++) {
		const char *param_name = node_params.l[i * 2 + 1].value.token.str;

		// name the parameter for fun
		LLVMSetValueName2(LLVMGetParam(func_ref, i), param_name, strlen(param_name));

		// add the parameter to the strmap
		strmap_set(&var_map_copy, param_name, & (struct var_map_entry) {
			.value = LLVMGetParam(func_ref, i),
			.type = param_types[i]
		}, sizeof(struct var_map_entry));
	}

	bool terminated = codegen_stmt_list(build, &children.l[3], &var_map_copy, func_map);

	if (!terminated && func_return_type.kind == TYPE_VOID)
		LLVMBuildRetVoid(build);

	struct function_info func_info = {
		.func = func_ref,
		.llvm_type = func_type,
		.num_params = num_params,
		.params = param_types,
		.return_type = func_return_type,
		.is_builtin = false
	};

	strmap_set(func_map, func_name, &func_info, sizeof(func_info));

	codegen_var_strmap_free(&var_map_copy);
	strmap_free(&var_map_copy);
}

void codegen_return(
	LLVMBuilderRef build,
	struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->node_type != AST_RETURN)
		ERROR_COMPILER();

	struct ast_node_list *list = &node->value.children;

	LLVMContextRef llvm_ctx = LLVMGetBuilderContext(build);
	LLVMBasicBlockRef ret_block = LLVMAppendBasicBlockInContext(
		llvm_ctx, func_ref, "return"
	);

	if (list->size == 0) {
		if (func_return_type.kind != TYPE_VOID) {
			fprintf(
				stderr,
				"[ERROR] line %zu: return type mismatch: expected %s, got void\n",
				node->line,
				type_to_str(func_return_type)
			);
			exit(1);
		}

		LLVMBuildBr(build, ret_block);
		LLVMPositionBuilderAtEnd(build, ret_block);
		LLVMBuildRetVoid(build);

		return;
	}

	if (list->size != 1 || list->l[0].node_type != AST_EXPR)
		ERROR_COMPILER();

	LLVMBuildBr(build, ret_block);
	LLVMPositionBuilderAtEnd(build, ret_block);

	LLVMValueRef value = codegen_expression(build, &list->l[0], var_map, func_map);

	if (func_return_type.kind == TYPE_VOID) {
		fprintf(
			stderr,
			"[ERROR] line %zu: return type mismatch: expected void, got %s\n",
			node->line,
			type_to_str(list->l[0].value_type)
		);
		exit(1);
	}

	if (!type_eq(func_return_type, list->l[0].value_type)) {
		fprintf(
			stderr,
			"[ERROR] line %zu: return type mismatch: expected %s, got %s\n",
			node->line,
			type_to_str(func_return_type),
			type_to_str(list->l[0].value_type)
		);
		exit(1);
	}

	LLVMBuildRet(build, value);
}


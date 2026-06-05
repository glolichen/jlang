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

	LLVMTypeRef type;
	bool is_builtin;
};

static void populate_builtin_funcs(
	LLVMContextRef llvm_ctx,
	struct strmap *func_map
) {
	struct function_info getchar_info = {
		.func = NULL,
		.type = LLVMFunctionType(LLVMInt8TypeInContext(llvm_ctx), NULL, 0, 0),
		.is_builtin = true
	};
	strmap_set(func_map, "getchar", &getchar_info, sizeof(getchar_info));
	
	LLVMTypeRef putchar_params[] = { LLVMInt32TypeInContext(llvm_ctx) };
	struct function_info putchar_info = {
		.func = NULL,
		.type = LLVMFunctionType(
			LLVMVoidTypeInContext(llvm_ctx),
			putchar_params,
			NUM_PARAMS(putchar_params),
			0
		),
		.is_builtin = true
	};

	strmap_set(func_map, "putchar", &putchar_info, sizeof(putchar_info));
}
void codegen_func_init(
	LLVMContextRef llvm_ctx,
	struct strmap *func_map
) {
	populate_builtin_funcs(llvm_ctx, func_map);
}

LLVMValueRef codegen_func_call(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	const char *func_name = node->value.children.l[0].value.token.str;

	if (
		strcmp(func_name, "to_i8") == 0 ||
		strcmp(func_name, "to_i16") == 0 ||
		strcmp(func_name, "to_i32") == 0 ||
		strcmp(func_name, "to_i64") == 0
	) {
		return types_convert_literal(build, node, var_map, func_map);
	}

	struct function_info *func_info = strmap_get(func_map, func_name);

	if (func_info == NULL) {
		fprintf(stderr, "line %zu: function %s not defined!\n", node->line, func_name);
		exit(1);
	}

	if (func_info->func == NULL) {
		if (!func_info->is_builtin)
			ERROR_COMPILER();

		// this will modify the value in the map as well
		LLVMModuleRef module = codegen_get_current_module();
		func_info->func = LLVMAddFunction(module, func_name, func_info->type);
	}
		
	struct ast_node_list *ast_params = &node->value.children.l[1].value.children;
	size_t ast_num_params = ast_params->size;

	// extract function parameter types for checking
	size_t num_params = LLVMCountParamTypes(func_info->type);

	if (ast_num_params != num_params) {
		fprintf(
			stderr,
			"line %zu: invalid number of parameters: expected %zu, got %zu\n",
			node->line, num_params, ast_num_params
		);
		exit(1);
	}

	LLVMTypeRef *param_types = malloc(num_params * sizeof(LLVMTypeRef));
	LLVMGetParamTypes(func_info->type, param_types);

	LLVMValueRef *params;
	if (ast_num_params == 0)
		params = NULL;
	else {
		params = malloc(ast_num_params * sizeof(LLVMValueRef));
		for (size_t i = 0; i < ast_num_params; i++) {
			params[i] = codegen_expression(build, &ast_params->l[i], var_map, func_map);

			// check types
			LLVMTypeRef got_type = ast_params->l[i].value_type;
			if (got_type != param_types[i]) {
				fprintf(
					stderr,
					"line %zu: type mismatch: expected %s, got %s\n",
					node->line,
					value_type_to_str(param_types[i], build),
					value_type_to_str(got_type, build)
				);
				exit(1);
			}
		}
	}

	LLVMValueRef out = LLVMBuildCall2(build, func_info->type, func_info->func, params, ast_num_params, "");

	free(param_types);
	free(params);

	node->value_type = LLVMGetReturnType(func_info->type);

	return out;
}

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

	LLVMTypeRef return_type = type_from_lex(
		build, children.l[0].value.token.type, true
	);

	const char *func_name = children.l[1].value.token.str;

	struct ast_node_list node_params = children.l[2].value.children;
	size_t num_params = node_params.size / 2;
	LLVMTypeRef *param_types = malloc(num_params * sizeof(LLVMTypeRef));
	for (size_t i = 0; i < num_params; i++) {
		param_types[i] = type_from_lex(
			build, node_params.l[i * 2].value.token.type, false
		);
	}

	LLVMTypeRef func_type = LLVMFunctionType(
		return_type, param_types, num_params, 0
	);

	LLVMValueRef func = LLVMAddFunction(module, func_name, func_type);

	LLVMContextRef llvm_ctx = LLVMGetBuilderContext(build);

	LLVMBasicBlockRef entry_block = LLVMAppendBasicBlockInContext(
		llvm_ctx, func, "entry"
	);
	LLVMPositionBuilderAtEnd(build, entry_block);
	
	for (size_t i = 0; i < num_params; i++) {
		const char *param_name = node_params.l[i * 2 + 1].value.token.str;

		// name the parameter for fun
		LLVMSetValueName2(LLVMGetParam(func, i), param_name, strlen(param_name));

		// add the parameter to the strmap
		strmap_set(&var_map_copy, param_name, & (struct var_map_entry) {
			.value = LLVMGetParam(func, i),
			.type = param_types[i]
		}, sizeof(struct var_map_entry));
	}

	codegen_stmt_list(build, &children.l[3], &var_map_copy, func_map);

	struct function_info func_info = {
		.func = func,
		.type = func_type,
		.is_builtin = false
	};

	strmap_set(func_map, func_name, &func_info, sizeof(func_info));

	strmap_free(&var_map_copy);
	free(param_types);
}


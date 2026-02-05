#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen/function.h"
#include "codegen/codegen.h"
#include "codegen/expression.h"
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
void codegen_func_init(struct strmap *func_map) {
	populate_builtin_funcs(func_map);
}

LLVMValueRef codegen_func_call(
	LLVMBuilderRef build,
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
		LLVMModuleRef module = codegen_get_current_module();
		func_info->func = LLVMAddFunction(module, func_name, func_info->type);
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
			params[i] = codegen_expression(build, &ast_params->l[i], var_map, func_map);
	}

	LLVMValueRef out = LLVMBuildCall2(build, func_info->type, func_info->func, params, ast_param_num, "");
	free(params);

	return out;
}



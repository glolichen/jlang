#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <string.h>
#include <stdio.h>

#include "codegen/expression.h"
#include "types.h"
#include "error.h"
#include "ast.h"
#include "lex.h"
#include "utils/linkedlist.h"

// WARN: FIX change to print_type_str system
const char *type_to_str(struct type type) {
	if (type.kind == TYPE_PTR) {
		size_t num_ptrs = 0;
		struct type base_type = type;
		while (base_type.kind == TYPE_PTR) {
			base_type = *base_type.pointer.pointee;
			num_ptrs++;
		}

		const char *base_str = type_to_str(base_type);

		// 4 characters in "ptr " plus base length plus null terminator
		size_t len = num_ptrs * 4 + strlen(base_str) + 1;
		char *str = calloc(1, len * sizeof(char));

		for (size_t i = 0; i < num_ptrs; i++)
			strcat(str, "ptr ");
		strcat(str, base_str);

		printf("%s\n", str);

		free(str);

		return "ptr";
	}

	// while (LLVMGetTypeKind(type) == LLVMPointerTypeKind) {
	// 	printf("before = %p\n, opaque = %d\n", type, LLVMPointerTypeIsOpaque(type));
	// 	type = LLVMGetElementType(type);
	// 	printf("after = %p\n", type);
	// 	num_ptrs++;
	// }

	if (type.kind == TYPE_I8)
		return "i8";
	if (type.kind == TYPE_I16)
		return "i16";
	if (type.kind == TYPE_I32)
		return "i32";
	if (type.kind == TYPE_I64)
		return "i64";
	if (type.kind == TYPE_VOID)
		return "void";
	if (type.kind == TYPE_UNKNOWN)
		return "UNKNOWN";

	ERROR_COMPILER();
}

struct type type_from_ast_node(
	const struct ast_node *node,
	bool include_void
) {
	struct ast_node_list children = node->value.children;
	if (node->node_type != AST_TYPE || children.size == 0)
		ERROR_COMPILER();

	enum lex_token_type base_type_token =
		children.l[children.size - 1].value.token.type;

	enum type_kind base_type_kind;
	if (base_type_token == LEX_I8)
		base_type_kind = TYPE_I8;
	else if (base_type_token == LEX_I16)
		base_type_kind = TYPE_I16;
	else if (base_type_token == LEX_I32)
		base_type_kind = TYPE_I32;
	else if (base_type_token == LEX_I64)
		base_type_kind = TYPE_I64;
	else if (base_type_token == LEX_VOID) {
		if (children.size != 1 || !include_void)
			ERROR_COMPILER();
		return (struct type) {
			.kind = TYPE_VOID
		};
	}
	else
		ERROR_COMPILER();

	struct type *type = calloc(1, sizeof(struct type));
	type->kind = base_type_kind;
	type->pointer.pointee = NULL;

	for (size_t i = 1; i < children.size; i++) {
		struct type *inner_type = type;
		type = calloc(1, sizeof(struct type));
		type->kind = TYPE_PTR;
		type->pointer.pointee = inner_type;
	}

	struct type ret_type = *type;
	free(type);

	return ret_type;
}

LLVMTypeRef type_to_llvm_type(
	LLVMBuilderRef build,
	struct type type,
	bool include_void
) {
	LLVMContextRef llvm_ctx = LLVMGetBuilderContext(build);

	switch (type.kind) {
		case TYPE_I8:
			return LLVMInt8TypeInContext(llvm_ctx);
		case TYPE_I16:
			return LLVMInt16TypeInContext(llvm_ctx);
		case TYPE_I32:
			return LLVMInt32TypeInContext(llvm_ctx);
		case TYPE_I64:
			return LLVMInt64TypeInContext(llvm_ctx);
		case TYPE_PTR:
			return LLVMPointerTypeInContext(llvm_ctx, 0);
		case TYPE_VOID:
			if (include_void)
				return LLVMVoidTypeInContext(llvm_ctx);
		case TYPE_UNKNOWN:
			ERROR_COMPILER();
	}
}

bool type_eq(struct type a, struct type b) {
	if (a.kind != b.kind)
		return false;

	if (a.kind == TYPE_PTR)
		return type_eq(*a.pointer.pointee, *b.pointer.pointee);
	
	return true;
}

struct ll_list_node *free_list = NULL;

void add_to_list(void *ptr) {
	if (ptr == NULL)
		return;


	if (!free_list) {
		free_list = ll_new();

		// OK yes terrible hack
		// add a junk element so that the list is not NULL
		ll_add(&free_list, (void *) 0xdeadbeef);
	}

	struct ll_list_node *cur = free_list;
	while (cur != NULL) {
		if (cur->data == ptr)
			return;
		cur = cur->next;
	}

	ll_add(&free_list, ptr);
}

// if list == NULL, recursively free
// if list != NULL, add elements to the linked list
// making sure that the entries are unique
// does not actually free anything, only adds to list
void type_free(struct type *type) {
	if (type->kind == TYPE_PTR) {
		type_free(type->pointer.pointee);
		add_to_list(type->pointer.pointee);
		return;
	}
		add_to_list(type->pointer.pointee);
}


struct ll_list_node **get_free_type_list(void) {
	return &free_list;
}
void type_free_list(void) {
	struct ll_list_node *cur = free_list;
	while (cur != NULL) {
		if (cur->data != (void *) 0xdeadbeef)
			free(cur->data);
		cur = cur->next;
	}
}

LLVMValueRef types_convert_literal(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
) {
	LLVMContextRef llvm_ctx = LLVMGetBuilderContext(build);

	const char *func_name = node->value.children.l[0].value.token.str;

	enum type_kind type_kind;
	LLVMTypeRef llvm_type;
	if (strcmp(func_name, "toi8") == 0) {
		type_kind = TYPE_I8;
		llvm_type = LLVMInt8TypeInContext(llvm_ctx);
	}
	else if (strcmp(func_name, "toi16") == 0) {
		type_kind = TYPE_I16;
		llvm_type = LLVMInt16TypeInContext(llvm_ctx);
	}
	else if (strcmp(func_name, "toi32") == 0) {
		type_kind = TYPE_I32;
		llvm_type = LLVMInt32TypeInContext(llvm_ctx);
	}
	else if (strcmp(func_name, "toi64") == 0) {
		type_kind = TYPE_I64;
		llvm_type = LLVMInt64TypeInContext(llvm_ctx);
	}
	else
		ERROR_COMPILER();

	struct ast_node_list *ast_params = &node->value.children.l[1].value.children;

	if (ast_params->size != 1)
		goto wrong_params;

	struct ast_node *expr = &ast_params->l[0];
	if (expr->node_type != AST_EXPR)
		goto wrong_params;

	node->value_type = (struct type) { .kind = type_kind };

	LLVMValueRef expr_value = codegen_expression(build, expr, var_map, func_map);
	return LLVMBuildIntCast2(build, expr_value, llvm_type, true, "intcast");

wrong_params:
	fprintf(
		stderr,
		"[ERROR] line %zu: function %s accepts 1 number literal parameter\n",
		node->line, func_name
	);
	exit(1);
}


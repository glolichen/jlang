#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <inttypes.h>
#include <stdio.h>

#include "codegen/variable.h"
#include "codegen/expression.h"
#include "utils/strmap.h"
#include "types.h"
#include "ast.h"
#include "error.h"
#include "lex.h"

void codegen_var_declaration(
	LLVMBuilderRef build,
	struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	(void) func_map;

	if (node->node_type != AST_VAR_DECLARATION)
		ERROR_COMPILER();

	struct ast_node_list *list = &node->value.children;

	if (
		list->size != 2 ||
		list->l[0].node_type != AST_TYPE ||
		list->l[1].node_type != AST_LEAF
	) {
		ERROR_COMPILER();
	}

	struct type type = type_from_ast_node(&list->l[0], false);
	LLVMTypeRef llvm_type = type_to_llvm_type(build, type, false);

	const struct lex_token *ident = &list->l[1].value.token;

	struct var_map_entry rhs = {
		.value = LLVMConstInt(llvm_type, 0, 0),
		.type = type
	};

	strmap_set(var_map, ident->str, &rhs, sizeof(struct var_map_entry));
}

void codegen_assignment(
	LLVMBuilderRef build,
	struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	if (node->node_type != AST_ASSIGN)
		ERROR_COMPILER();

	struct ast_node_list *list = &node->value.children;

	if (list->size != 2 || list->l[0].node_type != AST_LEAF)
		ERROR_COMPILER();

	struct lex_token *ident = &list->l[0].value.token;
	LLVMValueRef rhs = codegen_expression(build, &list->l[1], var_map, func_map);

	struct var_map_entry *entry = strmap_get(var_map, ident->str);
	if (entry == NULL) {
		fprintf(
			stderr,
			"[ERROR] line %zu: variable %s is not declared",
			node->line,
			ident->str
		);
		exit(1);
	}
	if (!type_eq(entry->type, list->l[1].value_type)) {
		fprintf(
			stderr,
			"[ERROR] line %zu: type mismatch: expected %s, got %s\n",
			node->line,
			type_to_str(entry->type),
			type_to_str(list->l[1].value_type)
		);
		exit(1);
	}
	entry->value = rhs;
}

void codegen_var_strmap_free(struct strmap *var_map) {
	for (uint64_t i = 0; i < var_map->bucket_count; i++) {
		struct strmap_list_node *node = var_map->list[i];
		while (node != NULL) {
			struct var_map_entry var = *(struct var_map_entry *) node->value;
			type_free(&var.type);
			node = node->next;
		}
	}
}


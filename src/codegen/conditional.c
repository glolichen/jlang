#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <inttypes.h>
#include <string.h>

#include "codegen/conditional.h"
#include "codegen/statement.h"
#include "codegen/expression.h"
#include "codegen/variable.h"
#include "utils/strmap.h"
#include "error.h"
#include "ast.h"

static void codegen_conditional_if_then(
	LLVMBuilderRef build,
	struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map,
	LLVMValueRef condition
) {
	LLVMContextRef llvm_ctx = LLVMGetBuilderContext(build);
	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(build));

	LLVMBasicBlockRef before_block = LLVMGetInsertBlock(build);
	LLVMBasicBlockRef then_block = LLVMAppendBasicBlockInContext(
		llvm_ctx, func, "ifthen"
	);
	LLVMBasicBlockRef after_block = LLVMAppendBasicBlockInContext(
		llvm_ctx, func, "ifcont"
	);

	LLVMBuildCondBr(build, condition, then_block, after_block);

	LLVMPositionBuilderAtEnd(build, then_block);
	struct strmap var_map_then = strmap_copy(var_map);
	bool terminated = codegen_stmt_list(
		build, &node->value.children.l[1], &var_map_then, func_map
	);
	if (!terminated)
		LLVMBuildBr(build, after_block);

	then_block = LLVMGetInsertBlock(build);

	LLVMPositionBuilderAtEnd(build, after_block);

	// iterate through ALREADY DEFINED variables and assign with phi nodes
	// but only do this if they have been modified by either block
	for (uint64_t i = 0; i < var_map->bucket_count; i++) {
		struct strmap_list_node *cur_before = var_map->list[i];
		while (cur_before != NULL) {
			struct var_map_entry *entry_before = cur_before->value;
			struct var_map_entry *entry_then = strmap_get(&var_map_then, cur_before->str);

			// if conditional does not affect value, no need for phi
			if (entry_before->value == entry_then->value) {
				cur_before = cur_before->next;
				continue;
			}

			LLVMValueRef phi = LLVMBuildPhi(build, entry_before->type, "ifphitmp");

			if (!terminated)
				LLVMAddIncoming(phi, &entry_then->value, &then_block, 1);
			LLVMAddIncoming(phi, &entry_before->value, &before_block, 1);

			strmap_set(var_map, cur_before->str, & (struct var_map_entry) {
				.value = phi, .type = entry_before->type
			}, sizeof(struct var_map_entry));

			cur_before = cur_before->next;
		}
	}
	
	strmap_free(&var_map_then);
}

static void codegen_conditional_if_then_else(
	LLVMBuilderRef build,
	struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map,
	LLVMValueRef condition
) {
	LLVMContextRef llvm_ctx = LLVMGetBuilderContext(build);
	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(build));

	LLVMBasicBlockRef then_block = LLVMAppendBasicBlockInContext(
		llvm_ctx, func, "ifthen"
	);
	LLVMBasicBlockRef else_block = LLVMAppendBasicBlockInContext(
		llvm_ctx, func, "ifelse"
	);
	LLVMBasicBlockRef merge_block = LLVMAppendBasicBlockInContext(
		llvm_ctx, func, "ifcont"
	);

	LLVMBuildCondBr(build, condition, then_block, else_block);

	// generate then block and add merge block to terminate it
	LLVMPositionBuilderAtEnd(build, then_block);
	struct strmap var_map_then = strmap_copy(var_map);
	bool then_terminated = codegen_stmt_list(
		build, &node->value.children.l[1], &var_map_then, func_map
	);
	if (!then_terminated)
		LLVMBuildBr(build, merge_block);

	then_block = LLVMGetInsertBlock(build);

	// generate else block and add merge block to terminate it
	LLVMPositionBuilderAtEnd(build, else_block);
	struct strmap var_map_else = strmap_copy(var_map);
	bool else_terminated = codegen_stmt_list(
		build, &node->value.children.l[2], &var_map_else, func_map
	);
	if (!else_terminated)
		LLVMBuildBr(build, merge_block);

	else_block = LLVMGetInsertBlock(build);

	// deal with merge blocks and add phi nodes
	LLVMPositionBuilderAtEnd(build, merge_block);

	// iterate through ALREADY DEFINED variables and assign with phi nodes
	// but only do this if they have been modified by either block
	for (uint64_t i = 0; i < var_map->bucket_count; i++) {
		struct strmap_list_node *cur = var_map->list[i];
		while (cur != NULL) {
			struct var_map_entry *entry_cur = cur->value;
			struct var_map_entry *entry_then = strmap_get(&var_map_then, cur->str);
			struct var_map_entry *entry_else = strmap_get(&var_map_else, cur->str);

			// if conditional does not affect value, no need for phi
			if (
				entry_cur->value == entry_then->value &&
				entry_cur->value == entry_else->value
			) {
				cur = cur->next;
				continue;
			}

			LLVMValueRef phi = LLVMBuildPhi(build, entry_cur->type, "ifelsephitmp");

			if (!then_terminated)
				LLVMAddIncoming(phi, &entry_then->value, &then_block, 1);
			if (!else_terminated)
				LLVMAddIncoming(phi, &entry_else->value, &else_block, 1);

			strmap_set(var_map, cur->str, & (struct var_map_entry) {
				.value = phi, .type = entry_cur->type
			}, sizeof(struct var_map_entry));

			cur = cur->next;
		}
	}
	
	strmap_free(&var_map_then);
	strmap_free(&var_map_else);
}

// will modify var_map using phi nodes
void codegen_conditional(
	LLVMBuilderRef build,
	struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	LLVMContextRef llvm_ctx = LLVMGetBuilderContext(build);
	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(build));

	LLVMBasicBlockRef cond_block = LLVMAppendBasicBlockInContext(
		llvm_ctx, func, "ifcond"
	);
	LLVMBuildBr(build, cond_block);
	LLVMPositionBuilderAtEnd(build, cond_block);

	LLVMValueRef condition = codegen_expression(
		build,
		&node->value.children.l[0],
		var_map, func_map
	);

	if (condition == NULL)
		ERROR_COMPILER();

	condition = LLVMBuildICmp(
		build, LLVMIntNE, condition,
		LLVMConstInt(node->value.children.l[0].value_type, 0, 0),
		"ifcmptmp"
	);

	// 2 = no else (if then)
	if (node->value.children.size == 2)
		codegen_conditional_if_then(build, node, var_map, func_map, condition);
	// 3 = has else (if then else)
	else if (node->value.children.size == 3)
		codegen_conditional_if_then_else(build, node, var_map, func_map, condition);
	else
		ERROR_COMPILER();
}


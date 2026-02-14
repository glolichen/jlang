#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen/conditional.h"
#include "codegen/statement.h"
#include "codegen/expression.h"
#include "utils/strmap.h"
#include "ast.h"

static void codegen_conditional_if_then(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map,
	LLVMValueRef condition
) {
	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(build));

	LLVMBasicBlockRef before_block = LLVMGetInsertBlock(build);
	LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func, "ifthen");
	LLVMBasicBlockRef after_block = LLVMAppendBasicBlock(func, "ifcont");

	LLVMBuildCondBr(build, condition, then_block, after_block);

	LLVMPositionBuilderAtEnd(build, then_block);
	struct strmap var_map_then = strmap_copy(var_map);
	bool terminated = codegen_stmt_list(build, &node->value.children.l[1], &var_map_then, func_map);
	if (!terminated)
		LLVMBuildBr(build, after_block);
	then_block = LLVMGetInsertBlock(build);

	LLVMPositionBuilderAtEnd(build, after_block);

	// iterate through ALREADY DEFINED variables and assign with phi nodesconditio
	// but only do this if they have been modified by either block
	for (uint64_t i = 0; i < var_map->bucket_count; i++) {
		struct strmap_list_node *cur_before = var_map->list[i];
		while (cur_before != NULL) {
			LLVMValueRef value_before = *(LLVMValueRef *) cur_before->value;
			LLVMValueRef value_then = *(LLVMValueRef *) strmap_get(&var_map_then, cur_before->str);

			// if conditional does not affect value, no need for phi
			if (value_before == value_then) {
				cur_before = cur_before->next;
				continue;
			}

			LLVMValueRef phi = LLVMBuildPhi(build, LLVMInt32Type(), "ifphitmp");
			LLVMAddIncoming(phi, &value_then, &then_block, 1);
			LLVMAddIncoming(phi, &value_before, &before_block, 1);

			strmap_set(var_map, cur_before->str, &phi, sizeof(LLVMValueRef));

			cur_before = cur_before->next;
		}
	}
	
	strmap_free(&var_map_then);
}

static void codegen_conditional_if_then_else(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map,
	LLVMValueRef condition
) {
	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(build));

	LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func, "ifthen");
	LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(func, "ifelse");
	LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(func, "ifcont");

	LLVMBuildCondBr(build, condition, then_block, else_block);

	// generate then block and add merge block to terminate it
	LLVMPositionBuilderAtEnd(build, then_block);
	struct strmap var_map_then = strmap_copy(var_map);
	codegen_stmt_list(build, &node->value.children.l[1], &var_map_then, func_map);

	LLVMBuildBr(build, merge_block);
	then_block = LLVMGetInsertBlock(build);

	// generate else block and add merge block to terminate it
	LLVMPositionBuilderAtEnd(build, else_block);
	struct strmap var_map_else = strmap_copy(var_map);
	codegen_stmt_list(build, &node->value.children.l[2], &var_map_else, func_map);

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

			// if conditional does not affect value, no need for phi
			if (value_cur == value_then && value_cur == value_else) {
				cur = cur->next;
				continue;
			}

			LLVMValueRef phi = LLVMBuildPhi(build, LLVMInt32Type(), "ifelsephitmp");
			LLVMAddIncoming(phi, &value_then, &then_block, 1);
			LLVMAddIncoming(phi, &value_else, &else_block, 1);

			strmap_set(var_map, cur->str, &phi, sizeof(LLVMValueRef));

			cur = cur->next;
		}
	}
	
	strmap_free(&var_map_then);
	strmap_free(&var_map_else);
}

// will modify var_map using phi nodes
void codegen_conditional(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(build));

	LLVMBasicBlockRef cond_block = LLVMAppendBasicBlock(func, "ifcond");
	LLVMBuildBr(build, cond_block);
	LLVMPositionBuilderAtEnd(build, cond_block);

	LLVMValueRef condition = codegen_expression(
		build,
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

	// 2 = no else (if then)
	if (node->value.children.size == 2)
		codegen_conditional_if_then(build, node, var_map, func_map, condition);
	// 3 = has else (if then else)
	else if (node->value.children.size == 3)
		codegen_conditional_if_then_else(build, node, var_map, func_map, condition);
	else {
		fprintf(stderr, "ERROR! (20)");
		exit(1);
	}
}


#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen/forloop.h"
#include "codegen/assignment.h"
#include "codegen/expression.h"
#include "codegen/statement.h"
#include "utils/strmap.h"
#include "ast.h"

struct codegen_for_loop_context loop_context = { NULL, NULL };

const struct codegen_for_loop_context *codegen_get_for_loop_context(void) {
	if (loop_context.body_block == NULL || loop_context.after_phi_block == NULL)
		return NULL;
	return &loop_context;
}

void codegen_for_loop(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	// for nested loops, restore previous context
	struct codegen_for_loop_context before_ctx = loop_context;

	// adding the initial value both var_map and var_map_loop
	// this is so that we can use a phi when setting the variable in the body of the loop
	// this can be removed from the var_map at the end (if variable not declared/defined before)
	// FIXME: have it removed, if it's not already declared

	// variable assigned in: for ([here]; ...; ...)
	// currently only one variable, if we add support for multiple assignments (a = 0, b = 0, ...)
	// this will have to become an array/list of char *'s
	const char *loop_assign_var = NULL;
	if (node->value.children.l[0].value.children.size != 0) {
		codegen_assignment(build, &node->value.children.l[0], var_map, func_map);
		// get string in the AST
		// ok that these points are the same, the AST isn't freed
		loop_assign_var = node->value.children.l[0].value.children.l[0].value.token.str;
	}
	struct strmap var_map_loop = strmap_copy(var_map);

	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(build));

	// create references to block before for loop, for loop body, and after loop
	LLVMBasicBlockRef before_block = LLVMGetInsertBlock(build);
	LLVMBasicBlockRef body_block = LLVMAppendBasicBlock(func, "forbody");
	LLVMBasicBlockRef cond_block = LLVMAppendBasicBlock(func, "forcond");
	LLVMBasicBlockRef after_phi_block = LLVMAppendBasicBlock(func, "forafterphi");

	loop_context.body_block = body_block;
	loop_context.after_phi_block = after_phi_block;
	loop_context.for_node = node;

	// evaluate end condition to decide whether to execute loop at all
	// if the for loop condition is left blank, always true
	LLVMValueRef end_condition;
	if (node->value.children.l[1].value.children.size == 0)
		end_condition = LLVMConstInt(LLVMInt1Type(), 1, 0);
	else {
		end_condition = codegen_expression(
			build,
			&node->value.children.l[1],
			&var_map_loop, func_map
		);
		if (end_condition == NULL) {
			fprintf(stderr, "ERROR! (21)\n");
			exit(1);
		}
		end_condition = LLVMBuildICmp(
			build, LLVMIntNE, end_condition,
			LLVMConstInt(LLVMInt32Type(), 0, 0),
			"forcmptmp"
		);
	}
	// if satisfies condition, run loop
	// otherwise, go to loop after immediately
	LLVMBuildCondBr(build, end_condition, body_block, after_phi_block);

	LLVMPositionBuilderAtEnd(build, body_block);

	// create phi nodes for every variable
	// value changes depending on whether we are just entering or
	// if loop body has already executed previously
	// make new string map to save phi nodes for each variable
	// TODO: this doesn't have to be a hash table, a list of pairs is ok
	struct strmap loop_phi_nodes = strmap_new();
	for (uint64_t i = 0; i < var_map->bucket_count; i++) {
		// linked list current node for var_map before loop (in "var_map" variable)
		struct strmap_list_node *cur_before = var_map->list[i];
		while (cur_before != NULL) {
			LLVMValueRef phi = LLVMBuildPhi(build, LLVMInt32Type(), "forbodyphitmp");

			// value before = value from before hte for loop
			LLVMValueRef value_before = *(LLVMValueRef *) cur_before->value;

			// if predecessor to current block is the before_block, this is the first iteration
			// then need to use the value_before (this is how a phi node works)
			LLVMAddIncoming(phi, &value_before, &before_block, 1);

			// save phi node
			// need separate map because the value in var_map_loop will be modified
			strmap_set(&loop_phi_nodes, cur_before->str, &phi, sizeof(LLVMValueRef));
			strmap_set(&var_map_loop, cur_before->str, &phi, sizeof(LLVMValueRef));

			cur_before = cur_before->next;
		}
	}
	loop_context.loop_phi_nodes = &loop_phi_nodes;

	codegen_stmt_list(build, &node->value.children.l[3], &var_map_loop, func_map); 

	// check loop condition to decide ...
	LLVMBuildBr(build, cond_block);
	LLVMPositionBuilderAtEnd(build, cond_block);

	if (node->value.children.l[2].value.children.size != 0)
		codegen_assignment(build, &node->value.children.l[2], &var_map_loop, func_map);

	if (node->value.children.l[1].value.children.size == 0)
		end_condition = LLVMConstInt(LLVMInt1Type(), 1, 0);
	else {
		end_condition = codegen_expression(
			build,
			&node->value.children.l[1],
			&var_map_loop, func_map
		);
		if (end_condition == NULL) {
			fprintf(stderr, "ERROR! (21)\n");
			exit(1);
		}
		end_condition = LLVMBuildICmp(
			build, LLVMIntNE, end_condition,
			LLVMConstInt(LLVMInt32Type(), 0, 0),
			"forcmptmp"
		);
	}
	// ... whether to execute again (branch to loop body block again)
	// or to exit (branch to loop after block)
	LLVMBasicBlockRef loop_block_end = LLVMGetInsertBlock(build);
	LLVMBuildCondBr(build, end_condition, body_block, after_phi_block);

	LLVMPositionBuilderAtEnd(build, after_phi_block);

	// modify the original phi nodes created in the map earlier
	// add an possible incoming block, which is from itself
	// (if loop iterating again, the predecessor will be the loop_block)
	// in this case, the value is what is currently stored in the loop_block_end
	for (uint64_t i = 0; i < loop_phi_nodes.bucket_count; i++) {
		struct strmap_list_node *cur_phi = loop_phi_nodes.list[i];
		while (cur_phi != NULL) {
			LLVMValueRef phi = *(LLVMValueRef *) cur_phi->value;
			LLVMValueRef value_loop = *(LLVMValueRef *) strmap_get(&var_map_loop, cur_phi->str);
			LLVMAddIncoming(phi, &value_loop, &loop_block_end, 1);
			cur_phi = cur_phi->next;
		}
	}

	LLVMPositionBuilderAtEnd(build, after_phi_block);

	// iterate through ALREADY DEFINED variables and assign with phi nodes
	// value depends on whether the loop iterated at all
	for (uint64_t i = 0; i < var_map->bucket_count; i++) {
		struct strmap_list_node *cur_before = var_map->list[i];
		while (cur_before != NULL) {
			LLVMValueRef value_before = *(LLVMValueRef *) cur_before->value;
			LLVMValueRef value_loop = *(LLVMValueRef *) strmap_get(&var_map_loop, cur_before->str);

			if (value_before == value_loop) {
				cur_before = cur_before->next;
				continue;
			}

			// if loop did not iterate at all, predecessor is the before_block
			// if it did, it is the loop_block_end
			LLVMValueRef phi = LLVMBuildPhi(build, LLVMInt32Type(), "forafterphitmp");
			LLVMAddIncoming(phi, &value_before, &before_block, 1);
			LLVMAddIncoming(phi, &value_loop, &loop_block_end, 1);

			strmap_set(var_map, cur_before->str, &phi, sizeof(LLVMValueRef));

			cur_before = cur_before->next;
		}
	}

	// strmap_remove(var_map, loop_assign_var, false);

	strmap_free(&var_map_loop);
	strmap_free(&loop_phi_nodes);

	loop_context = before_ctx;
}

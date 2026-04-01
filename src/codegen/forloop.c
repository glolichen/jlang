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
#include "utils/linkedlist.h"
#include "utils/strmap.h"
#include "ast.h"

struct break_statement {
	struct strmap var_map;
	LLVMBasicBlockRef block;
};
struct for_loop_context {
	LLVMBasicBlockRef body_block, after_phi_block;
	struct strmap *loop_phi_nodes;
	const struct ast_node *for_node;
	struct ll_list_node *break_statements;
};

struct for_loop_context context;

void codegen_continue(
	LLVMBuilderRef build,
	struct strmap *var_map,
	struct strmap *func_map
) {
	// const struct for_loop_context *loop_ctx = codegen_get_for_loop_context();
	// if (loop_ctx == NULL) {
	// 	fprintf(stderr, "ERROR! continue/break outside loop\n");
	// 	exit(1);
	// }
	//
	// LLVMBasicBlockRef continue_block = LLVMGetInsertBlock(build);
	// for (uint64_t i = 0; i < loop_ctx->loop_phi_nodes->bucket_count; i++) {
	// 	struct strmap_list_node *cur_phi = loop_ctx->loop_phi_nodes->list[i];
	// 	while (cur_phi != NULL) {
	// 		LLVMValueRef phi = *(LLVMValueRef *) cur_phi->value;
	// 		LLVMValueRef value_cur = *(LLVMValueRef *) strmap_get(var_map, cur_phi->str);
	// 		LLVMAddIncoming(phi, &value_cur, &continue_block, 1);
	// 		cur_phi = cur_phi->next;
	// 	}
	// }
	//
	// const struct ast_node *for_node = loop_ctx->for_node;
	//
	// if (for_node->value.children.l[2].value.children.size != 0)
	// 	codegen_assignment(build, &for_node->value.children.l[2], var_map, func_map);
	//
	// LLVMValueRef end_condition;
	// if (for_node->value.children.l[1].value.children.size == 0)
	// 	end_condition = LLVMConstInt(LLVMInt1Type(), 1, 0);
	// else {
	// 	end_condition = codegen_expression(
	// 		build,
	// 		&for_node->value.children.l[1],
	// 		var_map, func_map
	// 	);
	// 	if (end_condition == NULL) {
	// 		fprintf(stderr, "ERROR! (21)\n");
	// 		exit(1);
	// 	}
	// 	end_condition = LLVMBuildICmp(
	// 		build, LLVMIntNE, end_condition,
	// 		LLVMConstInt(LLVMInt32Type(), 0, 0),
	// 		"forcmptmp"
	// 	);
	// }
	// LLVMBuildCondBr(build, end_condition, loop_ctx->body_block, loop_ctx->after_phi_block);

	// LLVMBuildBr(build, loop_ctx->cond_block);
}

void codegen_break(
	LLVMBuilderRef build,
	const struct strmap *var_map
) {
	// if body block is null then the context = {0} => called outside loop
	if (context.body_block == NULL) {
		fprintf(stderr, "ERROR! continue/break outside loop\n");
		exit(1);
	}

	struct break_statement *cur_break = malloc(sizeof(struct break_statement));
	cur_break->var_map = strmap_copy(var_map);
	cur_break->block = LLVMAppendBasicBlockInContext(
		LLVMGetBuilderContext(build),
		LLVMGetBasicBlockParent(LLVMGetInsertBlock(build)),
		"breakblock"
	);

	LLVMBuildBr(build, cur_break->block);
	LLVMPositionBuilderAtEnd(build, cur_break->block);

	LLVMBuildBr(build, context.after_phi_block);

	ll_add(&context.break_statements, cur_break);
}

void codegen_for_loop(
	LLVMBuilderRef build,
	const struct ast_node *node,
	struct strmap *var_map,
	struct strmap *func_map
) {
	LLVMContextRef llvm_ctx = LLVMGetBuilderContext(build);

	// for nested loops, restore previous context
	struct for_loop_context before_ctx = context;

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
	LLVMBasicBlockRef body_block = LLVMAppendBasicBlockInContext(
		llvm_ctx, func, "forbody"
	);
	LLVMBasicBlockRef cond_block = LLVMAppendBasicBlockInContext(
		llvm_ctx, func, "forcond"
	);
	LLVMBasicBlockRef after_phi_block = LLVMAppendBasicBlockInContext(
		llvm_ctx, func, "forafterphi"
	);

	context.break_statements = ll_new();
	context.body_block = body_block;
	context.after_phi_block = after_phi_block;
	context.for_node = node;

	// evaluate end condition to decide whether to execute loop at all
	// if the for loop condition is left blank, always true
	LLVMValueRef end_condition;
	if (node->value.children.l[1].value.children.size == 0)
		end_condition = LLVMConstInt(LLVMInt1TypeInContext(llvm_ctx), 1, 0);
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
			LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0, 0),
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
			LLVMValueRef phi = LLVMBuildPhi(
				build,
				LLVMInt32TypeInContext(llvm_ctx),
				"forbodyphitmp"
			);

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
	context.loop_phi_nodes = &loop_phi_nodes;

	codegen_stmt_list(build, &node->value.children.l[3], &var_map_loop, func_map); 

	// check loop condition to decide ...
	LLVMBuildBr(build, cond_block);
	LLVMPositionBuilderAtEnd(build, cond_block);

	if (node->value.children.l[2].value.children.size != 0)
		codegen_assignment(build, &node->value.children.l[2], &var_map_loop, func_map);

	if (node->value.children.l[1].value.children.size == 0)
		end_condition = LLVMConstInt(LLVMInt1TypeInContext(llvm_ctx), 1, 0);
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
			LLVMConstInt(LLVMInt32TypeInContext(llvm_ctx), 0, 0),
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

	// iterate through ALREADY DEFINED variables and assign with phi nodes
	// value depends on whether the loop iterated at all
	for (uint64_t i = 0; i < var_map->bucket_count; i++) {
		struct strmap_list_node *cur_before_loop = var_map->list[i];
		while (cur_before_loop != NULL) {
			LLVMValueRef value_before = *(LLVMValueRef *) cur_before_loop->value;
			LLVMValueRef value_loop = *(LLVMValueRef *) strmap_get(&var_map_loop, cur_before_loop->str);

			if (value_before == value_loop) {
				cur_before_loop = cur_before_loop->next;
				continue;
			}

			// if loop did not iterate at all, predecessor is the before_block
			// if it did, it is the loop_block_end
			LLVMValueRef phi = LLVMBuildPhi(
				build,
				LLVMInt32TypeInContext(llvm_ctx),
				"forafterphitmp"
			);
			LLVMAddIncoming(phi, &value_before, &before_block, 1);
			LLVMAddIncoming(phi, &value_loop, &loop_block_end, 1);

			struct ll_list_node *cur = context.break_statements;
			while (cur != NULL) {
				struct break_statement *cur_break = cur->data;

				LLVMValueRef value_at_break = *(LLVMValueRef *) strmap_get(
					&cur_break->var_map, cur_before_loop->str
				);
				LLVMAddIncoming(phi, &value_at_break, &cur_break->block, 1);

				cur = cur->next;
			}

			strmap_set(var_map, cur_before_loop->str, &phi, sizeof(LLVMValueRef));

			cur_before_loop = cur_before_loop->next;
		}
	}

	struct ll_list_node *cur = context.break_statements;
	while (cur != NULL) {
		struct break_statement *cur_break = cur->data;
		strmap_free(&cur_break->var_map);
		free(cur_break);
		cur = cur->next;
	}

	if (loop_assign_var != NULL)
		strmap_remove(var_map, loop_assign_var, false);

	strmap_free(&var_map_loop);
	strmap_free(&loop_phi_nodes);

	ll_free(&context.break_statements);

	context = before_ctx;
}


#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"
#include "ast.h"
#include "lex.h"

LLVMBuilderRef builder;

static LLVMValueRef codegen_number(const struct lex_token *token) {
	return LLVMConstInt(LLVMInt32Type(), token->literal.number, 0);
}

static LLVMValueRef codegen_expression(const struct ast_node *node);

static LLVMValueRef codegen_factor(const struct ast_node *node) {
	const struct ast_node *child = &node->value.children.l[0];

	if (child->type == AST_EXPR)
		return codegen_expression(child);

	if (child->value.token.type == LEX_NUMBER)
		return codegen_number(&child->value.token);

	// TODO: variable
	return NULL;
}

static LLVMValueRef codegen_term(const struct ast_node *node) {
	const struct ast_node_list *list = &node->value.children;
	LLVMValueRef lhs = codegen_factor(&list->l[0]);

	size_t i;
	for (i = 1; i < list->size - 1; i += 2) {
		if (list->l[i].type != AST_LEAF) {
			fprintf(stderr, "ERROR! (1)");
			exit(1);
		}

		LLVMValueRef rhs = codegen_factor(&list->l[i + 1]);

		if (list->l[i].value.token.type == LEX_STAR)
			lhs = LLVMBuildMul(builder, lhs, rhs, "multmp");
		else if (list->l[i].value.token.type == LEX_SLASH)
			lhs = LLVMBuildSDiv(builder, lhs, rhs, "multmp");
	}

	if (i != list->size) {
		fprintf(stderr, "ERROR! (2)");
		exit(1);
	}

	return lhs;
}

static LLVMValueRef codegen_expr_no_comp(const struct ast_node *node) {
	const struct ast_node_list *list = &node->value.children;

	bool first_is_negative = false;
	size_t i = 0;
	if (list->l[0].type == AST_LEAF) {
		if (list->l[0].value.token.type == LEX_MINUS)
			first_is_negative = true;
		i++;
	}

	LLVMValueRef lhs = codegen_term(&list->l[i]);
	if (first_is_negative)
		lhs = LLVMBuildMul(builder, lhs, LLVMConstInt(LLVMInt32Type(), -1, 0), "negtmp");

	for (i = i + 1; i < list->size - 1; i += 2) {
		if (list->l[i].type != AST_LEAF) {
			fprintf(stderr, "ERROR! (1)");
			exit(1);
		}

		LLVMValueRef rhs = codegen_term(&list->l[i + 1]);

		if (list->l[i].value.token.type == LEX_PLUS)
			lhs = LLVMBuildAdd(builder, lhs, rhs, "addtmp");
		else if (list->l[i].value.token.type == LEX_MINUS)
			lhs = LLVMBuildSub(builder, lhs, rhs, "subtmp");
	}

	if (i != list->size) {
		fprintf(stderr, "ERROR! (2)");
		exit(1);
	}

	return lhs;
}

static LLVMValueRef codegen_expression(const struct ast_node *node) {
	const struct ast_node_list *list = &node->value.children;

	// not comparison, only child is expr_no_comp
	if (list->size == 1)
		return codegen_expr_no_comp(&list->l[0]);

	// TODO: comparisons
	return NULL;
}

bool codegen(const char *name, const struct ast_node *root) {
    LLVMModuleRef mod = LLVMModuleCreateWithName(name);

    LLVMTypeRef param_types[] = { };
    LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 0, 0);
    LLVMValueRef sum = LLVMAddFunction(mod, "main", ret_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(sum, "entry");

    builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);

	LLVMValueRef tmp = codegen_expression(&root->value.children.l[0]);

	//    LLVMValueRef tmp = LLVMBuildAdd(
	// 	builder,
	// 	LLVMConstInt(LLVMInt32Type(), 30, 0),
	// 	LLVMConstInt(LLVMInt32Type(), 37, 0),
	// 	"tmp"
	// );

    LLVMBuildRet(builder, tmp);

    char *error = NULL;
    LLVMVerifyModule(mod, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);

	size_t len = strlen(name);
	len += 4; // ".bc" and a null terminator
	char *bitcode_filename = malloc(len * sizeof(char));
	strcpy(bitcode_filename, name);
	strcat(bitcode_filename, ".bc");

    // Write out bitcode to file
    if (LLVMWriteBitcodeToFile(mod, bitcode_filename) != 0)
        fprintf(stderr, "error writing bitcode to file, skipping\n");

	free(bitcode_filename);

    LLVMDisposeBuilder(builder);


	//    LLVMModuleRef mod = LLVMModuleCreateWithName(name);
	//
	//    LLVMTypeRef param_types[] = { LLVMInt32Type(), LLVMInt32Type() };
	//    LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);
	//    LLVMValueRef sum = LLVMAddFunction(mod, "sum", ret_type);
	//
	//    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(sum, "entry");
	//
	//    LLVMBuilderRef builder = LLVMCreateBuilder();
	//    LLVMPositionBuilderAtEnd(builder, entry);
	//    LLVMValueRef tmp = LLVMBuildAdd(builder, LLVMGetParam(sum, 0), LLVMGetParam(sum, 1), "tmp");
	//    LLVMBuildRet(builder, tmp);
	//
	//    char *error = NULL;
	//    LLVMVerifyModule(mod, LLVMAbortProcessAction, &error);
	//    LLVMDisposeMessage(error);
	//
	// size_t len = strlen(name);
	// len += 4; // ".bc" and a null terminator
	// char *bitcode_filename = malloc(len * sizeof(char));
	// strcpy(bitcode_filename, name);
	// strcat(bitcode_filename, ".bc");
	//
	//    // Write out bitcode to file
	//    if (LLVMWriteBitcodeToFile(mod, bitcode_filename) != 0)
	//        fprintf(stderr, "error writing bitcode to file, skipping\n");
	//
	// free(bitcode_filename);
	//
	//    LLVMDisposeBuilder(builder);
}



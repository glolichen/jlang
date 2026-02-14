#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen/codegen.h"
#include "codegen/function.h"
#include "codegen/statement.h"
#include "utils/strmap.h"
#include "ast.h"

LLVMModuleRef module = NULL;

LLVMModuleRef codegen_get_current_module(void) {
	return module;
}

bool codegen(const char *name, const struct ast_node *root) {
    module = LLVMModuleCreateWithName(name);

    LLVMTypeRef param_types[] = { };
    LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 0, 0);
    LLVMValueRef main_func = LLVMAddFunction(module, "main", ret_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry");

    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);

	// codegen_test(module, builder);

	struct strmap var_map = strmap_new(), func_map = strmap_new();
	codegen_func_init(&func_map);
	codegen_stmt_list(builder, &root->value.children.l[0], &var_map, &func_map);

	// char *error = NULL;
	// LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
	// LLVMDisposeMessage(error);

	size_t len = strlen(name);
	len += 4; // ".bc" and a null terminator
	char *bitcode_filename = malloc(len * sizeof(char));
	strcpy(bitcode_filename, name);
	strcat(bitcode_filename, ".bc");
	
    // Write out bitcode to file
	if (LLVMWriteBitcodeToFile(module, bitcode_filename) != 0)
		fprintf(stderr, "error writing bitcode to file, skipping\n");

	free(bitcode_filename);
	strmap_free(&var_map);
	strmap_free(&func_map);

    LLVMDisposeBuilder(builder);

	return true;
}



// void codegen_test(
// 	LLVMModuleRef mod, LLVMBuilderRef build
// ) {
// 	LLVMValueRef val_a = LLVMConstInt(LLVMInt32Type(), 0, 0);
// 	LLVMValueRef val_b = LLVMConstInt(LLVMInt32Type(), 0, 0);
//
// 	LLVMTypeRef getchar_type = LLVMFunctionType(LLVMInt8Type(), NULL, 0, 0);
// 	struct function_info getchar_info = {
// 		.func = NULL,
// 		.type = getchar_type,
// 		.is_builtin = true,
// 		.is_defined = false
// 	};
// 	LLVMValueRef getchar_func = LLVMAddFunction(mod, "getchar", getchar_type);
// 	LLVMValueRef val_input = LLVMBuildIntCast2(
// 		build,
// 		LLVMBuildCall2(
// 			build,
// 			getchar_type, getchar_func,
// 			NULL, 0, "getchartmp"
// 		),
// 		LLVMInt32Type(),
// 		true,
// 		"getcharcasttmp"
// 	);
//
// 	LLVMValueRef condition = LLVMBuildICmp(
// 		build, LLVMIntEQ,
// 		val_input,
// 		LLVMConstInt(LLVMInt32Type(), 48, 0),
// 		"ifcmptmp"
// 	);
//
// 	// Retrieve function.
// 	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(build));
//
// 	// Generate true/false expr and merge.
// 	LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(func, "then");
// 	LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(func, "else");
// 	LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(func, "ifcont");
//
// 	LLVMBuildCondBr(build, condition, then_block, else_block);
//
// 	// Generate 'then' block.
// 	LLVMPositionBuilderAtEnd(build, then_block);
// 	LLVMValueRef then_value = LLVMConstInt(LLVMInt32Type(), 10, 0);
//
// 	LLVMBuildBr(build, merge_block);
// 	then_block = LLVMGetInsertBlock(build);
//
// 	LLVMPositionBuilderAtEnd(build, else_block);
// 	LLVMValueRef else_value = LLVMConstInt(LLVMInt32Type(), 1, 0);
//
// 	LLVMBuildBr(build, merge_block);
// 	else_block = LLVMGetInsertBlock(build);
//
// 	LLVMPositionBuilderAtEnd(build, merge_block);
// 	LLVMValueRef phi = LLVMBuildPhi(build, LLVMInt32Type(), "phi");
// 	LLVMAddIncoming(phi, &then_value, &then_block, 1);
// 	LLVMAddIncoming(phi, &else_value, &else_block, 1);
//
//
// 	LLVMValueRef addtmp = LLVMBuildAdd(build, val_a, phi, "addtmp");
// 	LLVMBuildRet(build, addtmp);
// }


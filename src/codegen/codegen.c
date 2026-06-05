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
#include "codegen/master.h"
#include "codegen/statement.h"
#include "utils/strmap.h"
#include "ast.h"

LLVMModuleRef module = NULL;

LLVMModuleRef codegen_get_current_module(void) {
	return module;
}

bool codegen(const char *name, struct ast_node *root) {
	LLVMContextRef llvm_ctx = LLVMContextCreate();

    module = LLVMModuleCreateWithNameInContext(name, llvm_ctx);

    LLVMBuilderRef build = LLVMCreateBuilderInContext(llvm_ctx);

	struct strmap var_map = strmap_new(), func_map = strmap_new();
	codegen_func_init(llvm_ctx, &func_map);
	codegen_master(module, build, &root->value.children.l[0], &var_map, &func_map);

	char *error = NULL;
	LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
	LLVMDisposeMessage(error);

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

    LLVMDisposeBuilder(build);
    LLVMDisposeModule(module);
    LLVMContextDispose(llvm_ctx);

	return true;
}


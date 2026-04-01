#ifndef CODEGEN_H
#define CODEGEN_H

#include <llvm-c/Core.h>
#include <stdbool.h>
#include "ast.h"

LLVMModuleRef codegen_get_current_module(void);
bool codegen(const char *name, const struct ast_node *root);

#endif


#ifndef TYPES_H
#define TYPES_H

#include <llvm-c/Core.h>
#include "ast.h"

LLVMValueRef types_convert_literal(
	LLVMBuilderRef build,
	const struct ast_node *node
);

#endif


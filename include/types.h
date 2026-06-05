#ifndef TYPES_H
#define TYPES_H

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include "utils/strmap.h"
#include "lex.h"

// can't recursively #include ast.h
struct ast_node;

#define TYPE_I8(build) (LLVMInt8TypeInContext(LLVMGetBuilderContext(build)))
#define TYPE_I16(build) (LLVMInt16TypeInContext(LLVMGetBuilderContext(build)))
#define TYPE_I32(build) (LLVMInt32TypeInContext(LLVMGetBuilderContext(build)))
#define TYPE_I64(build) (LLVMInt64TypeInContext(LLVMGetBuilderContext(build)))
#define TYPE_VOID(build) (LLVMVoidTypeInContext(LLVMGetBuilderContext(build)))

#define TYPE_I8_CTX(ctx) (LLVMInt8TypeInContext(ctx))
#define TYPE_I16_CTX(ctx) (LLVMInt16TypeInContext(ctx))
#define TYPE_I32_CTX(ctx) (LLVMInt32TypeInContext(ctx))
#define TYPE_I64_CTX(ctx) (LLVMInt64TypeInContext(ctx))
#define TYPE_VOID_CTX(ctx) (LLVMVoidTypeInContext(ctx))

const char *value_type_to_str(LLVMTypeRef type, LLVMBuilderRef build);

LLVMTypeRef type_from_lex(
	LLVMBuilderRef build,
	enum lex_token_type token,
	bool include_void
);

LLVMValueRef types_convert_literal(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
);

#endif


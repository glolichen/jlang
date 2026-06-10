#ifndef TYPES_H
#define TYPES_H

#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include "utils/strmap.h"
#include "utils/linkedlist.h"

// can't recursively #include ast.h
struct ast_node;

enum type_kind {
    TYPE_VOID,
    // TYPE_BOOL,
    TYPE_UNKNOWN,
    TYPE_I8,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,
    // TYPE_F32,
    // TYPE_F64,
    TYPE_PTR,
    // TYPE_ARRAY,
    // TYPE_STRUCT,
    // TYPE_FUNCTION
};

struct type {
    enum type_kind kind;

    union {
        struct {
            struct type *pointee;
        } pointer;

        // struct {
        //     struct type *element;
        //     size_t count;
        // } array;
        //
        // struct {
        //     const char *name;
        //     struct type **fields;
        //     size_t num_fields;
        // } structure;

        // struct {
        //     struct type *return_type;
        //     struct type **params;
        //     size_t num_params;
        //     bool variadic;
        // } function;
    };
};

// #define TYPE_I8(build) (LLVMInt8TypeInContext(LLVMGetBuilderContext(build)))
// #define TYPE_I16(build) (LLVMInt16TypeInContext(LLVMGetBuilderContext(build)))
// #define TYPE_I32(build) (LLVMInt32TypeInContext(LLVMGetBuilderContext(build)))
// #define TYPE_I64(build) (LLVMInt64TypeInContext(LLVMGetBuilderContext(build)))
// #define TYPE_VOID(build) (LLVMVoidTypeInContext(LLVMGetBuilderContext(build)))
//
// #define TYPE_I8_CTX(ctx) (LLVMInt8TypeInContext(ctx))
// #define TYPE_I16_CTX(ctx) (LLVMInt16TypeInContext(ctx))
// #define TYPE_I32_CTX(ctx) (LLVMInt32TypeInContext(ctx))
// #define TYPE_I64_CTX(ctx) (LLVMInt64TypeInContext(ctx))
// #define TYPE_VOID_CTX(ctx) (LLVMVoidTypeInContext(ctx))

const char *type_to_str(struct type type);

struct type type_from_ast_node(
	const struct ast_node *node,
	bool include_void
);

LLVMTypeRef type_to_llvm_type(
	LLVMBuilderRef build,
	struct type type,
	bool include_void
);

bool type_eq(struct type a, struct type b);

void type_free(struct type *type);
struct ll_list_node **get_free_type_list(void);
void type_free_list(void);

LLVMValueRef types_convert_literal(
	LLVMBuilderRef build,
	struct ast_node *node,
	const struct strmap *var_map,
	struct strmap *func_map
);

#endif


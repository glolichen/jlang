#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>
#include "../ast.h"

bool codegen(const char *name, const struct ast_node *root);

#endif


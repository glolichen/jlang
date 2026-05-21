#include <stdlib.h>
#include <stdio.h>
#include "error.h"

_Noreturn void error_compiler(
	const char *fname,
	int lineno,
	const char *fxname
) {
	fprintf(
		stderr,
		"compiler error at %s:%d (function %s)\n",
		fname, lineno, fxname
	);
	exit(1);
}


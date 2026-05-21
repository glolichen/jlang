#ifndef ERROR_H
#define ERROR_H

#define ERROR_COMPILER() error_compiler(__FILE__, __LINE__, __func__)

_Noreturn void error_compiler(
	const char *fname,
	int lineno,
	const char *fxname
);

#endif


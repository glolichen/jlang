#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "lex.h"
#include "ast.h"
#include "parse.h"
#include "codegen.h"

size_t count_lines(FILE *file) {
	size_t lines = 0;
	for (char c = getc(file); c != EOF; c = getc(file)) {
		if (c == '\n')
			lines++;
	}

	// rewind sets errno on error
	errno = 0;
	rewind(file);
	if (errno != 0)
		return -1;

	return lines;
}

char *get_module_name(const char *filename) {
	size_t len = strlen(filename);

	size_t mod_name_len = len;
	for (size_t i = len; i >= 1; i--) {
		if (filename[i - 1] == '.') {
			mod_name_len = i - 1;
			break;
		}
	}

	char *module_name = malloc((mod_name_len + 1) * sizeof(char));
	if (module_name == NULL)
		return NULL;

	strncpy(module_name, filename, mod_name_len);
	module_name[mod_name_len] = 0;

	return module_name;
}

int main(int argc, const char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Error: need exactly two arguments\n");
		return 1;
	}

	FILE *infile = fopen(argv[1], "r");
	if (infile == NULL) {
		fprintf(stderr, "Error: failure reading file\n");
		return 1;
	}

	size_t num_lines = count_lines(infile);

	// check that these are not zero so that lines and line_lens later are not 0 length allocations
	// this is done to please the clang static analyzer
	if (num_lines == 0 || num_lines + 1 == 0) {
		fclose(infile);
		fprintf(stderr, "Error: empty file\n");
		return 1;
	}

	char **lines = malloc((num_lines + 1) * sizeof(char *));
	if (lines == NULL) {
		fclose(infile);
		fprintf(stderr, "Error: malloc failure\n");
		return 1;
	}

	for (size_t i = 0; i <= num_lines; i++)
		lines[i] = NULL;

	size_t *line_lens = malloc(num_lines * sizeof(size_t));
	if (line_lens == NULL) {
		fclose(infile);
		free(lines);
		fprintf(stderr, "Error: malloc failure\n");
		return 1;
	}

	size_t line_counter = 0;
	size_t buffer_size = 0, line_length = 0;
    while ((line_length = getline(&lines[line_counter], &buffer_size, infile)) != (size_t) -1)
		line_lens[line_counter++] = line_length;

	// note that getline will allocate a buffer even if it fails
	// this buffer needs to be freed
	// so we will keep track of the number of times it is called
	// that value will be line_counter + 1

	fclose(infile);

	struct lex_token_list token_list = lex_new_token_list();
	lex_scan(num_lines, (const char **) lines, line_lens, &token_list);

	// for (size_t i = 0; i < token_list.size; i++)
	// 	lex_print_token(&token_list.l[i]);

	struct ast_node root = ast_new_node(AST_ROOT);
	bool ok = parse(&token_list, (const char **) lines, &root);

	if (ok) {
		ast_print(&root);

		char *module_name = get_module_name(argv[1]);
		if (module_name == NULL) {
			fprintf(stderr, "Error: malloc failure\n");
			ok = false;
		}
		else {
			codegen(module_name, &root);
			free(module_name);
		}
	}

	ast_free_node(&root);
	// lex_free_token_list(&token_list);

	for (size_t i = 0; i <= line_counter; i++)
		free(lines[i]);
	free(lines);
	free(line_lens);

	return ok ? 0 : 1;
};


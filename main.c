#include "lex.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t count_lines(FILE *file) {
	size_t lines = 0;
	for (char c = getc(file); c != EOF; c = getc(file)) {
		if (c == '\n')
			lines++;
	}
	rewind(file);
	return lines;
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

	char **lines = malloc(num_lines * sizeof(char *));
	for (size_t i = 0; i < num_lines; i++)
		lines[i] = NULL;

	size_t *line_lens = malloc(num_lines * sizeof(size_t));

	int counter = 0;
	size_t buffer_size = 0, line_length = 0;
    while ((line_length = getline(&lines[counter], &buffer_size, infile)) != (size_t) -1)
		line_lens[counter++] = line_length - 1;

	fclose(infile);

	struct lex_token_list token_list = lex_new_token_list();
	// lex::ScanError scan_error = lex_scan(lines, tokens);
	lex_scan(num_lines, (const char **) lines, line_lens, &token_list);

	// switch (scan_error.type) {
	// 	case lex::SCAN_LINE_INT_OUT_OF_BOUNDS:
	// 		std::cerr << "Error: line " << scan_error.line << ": integer out of bounds";
	// 		return 1;
	// 	case lex::SCAN_LINE_OK:
	// 		;
	// }


	lex_free_token_list(&token_list);
	for (size_t i = 0; i < num_lines; i++)
		free(lines[i]);
	free(lines);
	free(line_lens);

	// for (lex::Token t : tokens) {
	// 	std::cout << t.to_string() << "\n";
	// }
	//
	// ast::Node root;
	// parse::parse(tokens, root);

	return 0;
};


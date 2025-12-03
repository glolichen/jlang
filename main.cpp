#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include "ast.hpp"
#include "lex.hpp"
#include "parse.hpp"

int main(int argc, const char *argv[]) {
	if (argc != 2) {
		std::cerr << "Error: need exactly two arguments\n";
		return 1;
	}

	std::string filename(argv[1]);
	std::ifstream fin(filename);
	if (fin.fail()) {
		std::cerr << "Error: failure reading file\n";
		return 1;
	}

	std::vector<std::string> lines;
	std::string line;
	while (std::getline(fin, line))
		lines.push_back(line);

	fin.close();

	std::vector<lex::Token> tokens;
	lex::ScanError scan_error = lex::scan(lines, tokens);

	switch (scan_error.type) {
		case lex::SCAN_LINE_INT_OUT_OF_BOUNDS:
			std::cerr << "Error: line " << scan_error.line << ": integer out of bounds";
			return 1;
		case lex::SCAN_LINE_OK:
			;
	}

	for (lex::Token t : tokens) {
		std::cout << t.to_string() << "\n";
	}

	ast::Node root;
	parse::parse(tokens, root);

	return 0;
};


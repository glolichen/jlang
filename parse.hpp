#ifndef PARSE_HPP
#define	PARSE_HPP

#include <vector>
#include "lex.hpp"
#include "ast.hpp"

namespace parse {
	void parse(const std::vector<lex::Token> &tokens, ast::Node &root);
}

#endif

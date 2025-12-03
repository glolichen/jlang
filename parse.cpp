#include <iostream>
#include <algorithm>
#include <vector>

#include "lex.hpp"
#include "parse.hpp"
#include "ast.hpp"

static std::vector<lex::Token>::const_iterator cur;

// move onto the next lexeme
static void next() {
	cur++;
}

// check if current lexeme is OK (in the list)
static bool is_type(const std::vector<lex::TokenType> &accepts) {
	return std::find(accepts.begin(), accepts.end(), cur->type) != accepts.end();
}
static bool is_type(lex::TokenType t) {
	return is_type(std::vector<lex::TokenType> { t });
}

// same as accept but will error on failure
static bool expect(const std::vector<lex::TokenType> &accepts) {
	if (is_type(accepts))
		return true;
	std::cerr << "ERROR! (1)\n";
	return false;
}
static bool expect(lex::TokenType t) {
	return expect(std::vector<lex::TokenType> { t });
}

static void expression(ast::Node &node);

static void factor(ast::Node &node) {
	if (is_type({ lex::NUMBER, lex::IDENTIFIER })) {
		node.insert_leaf(*cur);
		next();
		return;
	}
	if (is_type(lex::LEFT_PAREN)) {
		node.insert_leaf(*cur);
		next();

		expression(node.insert_node(ast::NODE_EXPR));

		expect(lex::RIGHT_PAREN);
		node.insert_leaf(*cur);
		next();
	}
	std::cerr << "ERROR! (2)\n";
}

static void term(ast::Node &node) {
	factor(node.insert_node(ast::NODE_FACTOR));
	while (cur->type == lex::STAR || cur->type == lex::SLASH) {
		node.insert_leaf(*cur);
		next();
		factor(node.insert_node(ast::NODE_FACTOR));
	}
}

static void expression(ast::Node &node) {
	if (cur->type == lex::PLUS || cur->type == lex::MINUS)
		next();
	term(node.insert_node(ast::NODE_TERM));
	while (cur->type == lex::PLUS || cur->type == lex::MINUS) {
		node.insert_leaf(*cur);
		next();
		term(node.insert_node(ast::NODE_TERM));
	}
}

static void goal(ast::Node &node) {
	expression(node);
}

void parse::parse(const std::vector<lex::Token> &tokens, ast::Node &root) {
	cur = tokens.begin();
	goal(root);
}


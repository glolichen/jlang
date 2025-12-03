#include <cstddef>
#include <iostream>
#include <variant>
#include <string>
#include <vector>
#include <map>

#include "lex.hpp"

static const std::map<std::string, lex::TokenType> KEYWORDS = {
	{ "else", lex::ELSE },
	{ "false", lex::FALSE },
	{ "for", lex::FOR },
	{ "if", lex::IF },
	{ "return", lex::RETURN },
	{ "true", lex::TRUE },
	{ "while", lex::WHILE },
	{ "int", lex::INT },
	{ "char", lex::CHAR },
};

static const std::map<std::string, lex::TokenType> DELIMS = {
	{ " ", lex::NOTHING },
	{ "\t", lex::NOTHING },
	{ "\n", lex::NOTHING },
	{ "(", lex::LEFT_PAREN },
	{ ")", lex::RIGHT_PAREN },
	{ "{", lex::LEFT_BRACE },
	{ "}", lex::RIGHT_BRACE },
	{ ",", lex::COMMA },
	{ ".", lex::DOT },
	{ "-", lex::MINUS },
	{ "+", lex::PLUS },
	{ ";", lex::SEMICOLON },
	{ "/", lex::SLASH },
	{ "*", lex::STAR },
	{ "!", lex::BANG },
	{ "!=", lex::BANG_EQUAL },
	{ "=", lex::EQUAL },
	{ "==", lex::EQUAL_EQUAL },
	{ ">", lex::GREATER },
	{ ">=", lex::GREATER_EQUAL },
	{ "<", lex::LESS },
	{ "<=", lex::LESS_EQUAL },
	{ "&&", lex::AND },
	{ "||", lex::OR },
};

static size_t get_longest_key(const std::map<std::string, lex::TokenType> &map) {
	size_t longest = 0;
	for (const auto &[key, _] : map)
		longest = std::max<size_t>(key.size(), longest);
	return longest;
}

static const size_t max_keyword_len = get_longest_key(KEYWORDS);
static const size_t max_delim_len = get_longest_key(DELIMS);

static lex::ScanError scan_line(int line_num, std::vector<lex::Token> &tokens, const std::string &line) {
	size_t start = 0;
	for (size_t i = 0; i < line.size(); i++) {
		for (size_t j = 1; j <= max_delim_len && j <= line.size() - i; j++) {
			std::string delim = line.substr(i, j);
			auto delim_it = DELIMS.find(delim);
			if (delim_it == DELIMS.end())
				continue;
			
			if (start != i) {
				std::string prev_token = line.substr(start, i - start);
				auto kw_it = KEYWORDS.find(prev_token);
				lex::TokenType type = kw_it == KEYWORDS.end() ? lex::IDENTIFIER : kw_it->second;
				tokens.push_back(lex::Token { type, std::monostate {}, prev_token, line_num });
			}

			if (delim_it->second != lex::NOTHING)
				tokens.push_back(lex::Token { delim_it->second, std::monostate {}, delim, line_num });

			start = i + 1;
			i += j - 1;
		}
	}

	for (lex::Token &token : tokens) {
		int num;
		try {
			num = std::stoi(token.str);
		}
        catch (const std::invalid_argument &ex) {
			continue;
        }
        catch (const std::out_of_range &ex) {
			return { lex::SCAN_LINE_INT_OUT_OF_BOUNDS, token.line };
		}
		token.type = lex::NUMBER;
		token.literal = num;
	}

	return { lex::SCAN_LINE_OK, 0 };
}

lex::ScanError lex::scan(const std::vector<std::string> &lines, std::vector<lex::Token> &tokens) {
	for (size_t i = 0; i < lines.size(); i++) {
		std::string line = lines[i];
		line.append("\n");
		lex::ScanError ret = scan_line(i + 1, tokens, line);
		if (ret.type != lex::SCAN_LINE_OK)
			return ret;
	}
	return { lex::SCAN_LINE_OK, 0 };
}


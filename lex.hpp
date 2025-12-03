#ifndef LEX_HPP
#define LEX_HPP

#include <codecvt>
#include <variant>
#include <sstream>
#include <string>
#include <vector>

namespace lex {
	// https://craftinginterpreters.com/scanning.html
	enum TokenType {
		LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
		COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

		// One or two character tokens.
		BANG, BANG_EQUAL,
		EQUAL, EQUAL_EQUAL,
		GREATER, GREATER_EQUAL,
		LESS, LESS_EQUAL,

		// Literals.
		IDENTIFIER, STRING, NUMBER,

		AND, ELSE, FALSE, FOR, IF, OR,
		RETURN, TRUE, WHILE,

		INT, CHAR,

		NOTHING
	};

	static std::string token_type_to_str(TokenType type) {
		switch (type) {
			case LEFT_PAREN:
				return "LEFT_PAREN";
			case RIGHT_PAREN:
				return "RIGHT_PAREN";
			case LEFT_BRACE:
				return "LEFT_BRACE";
			case RIGHT_BRACE:
				return "RIGHT_BRACE";
			case COMMA:
				return "COMMA";
			case DOT:
				return "DOT";
			case MINUS:
				return "MINUS";
			case PLUS:
				return "PLUS";
			case SEMICOLON:
				return "SEMICOLON";
			case SLASH:
				return "SLASH";
			case STAR:
				return "STAR";
			case BANG:
				return "BANG";
			case BANG_EQUAL:
				return "BANG_EQUAL";
			case EQUAL:
				return "EQUAL";
			case EQUAL_EQUAL:
				return "EQUAL_EQUAL";
			case GREATER:
				return "GREATER";
			case GREATER_EQUAL:
				return "GREATER_EQUAL";
			case LESS:
				return "LESS";
			case LESS_EQUAL:
				return "LESS_EQUAL";
			case IDENTIFIER:
				return "IDENTIFIER";
			case STRING:
				return "STRING";
			case NUMBER:
				return "NUMBER";
			case AND:
				return "AND";
			case ELSE:
				return "ELSE";
			case FALSE:
				return "FALSE";
			case FOR:
				return "FOR";
			case IF:
				return "IF";
			case OR:
				return "OR";
			case RETURN:
				return "RETURN";
			case TRUE:
				return "TRUE";
			case WHILE:
				return "WHILE";
			case INT:
				return "INT";
			case CHAR:
				return "CHAR";
			case NOTHING:
				return "NOTHING";
		}
		return "";
	}

	struct Token {
		TokenType type;
		std::variant<std::monostate, int, std::string> literal;
		std::string str;
		int line;

		std::string to_string() {
			std::stringstream ss;
			ss << "{ type = " << token_type_to_str(type) << ", literal = ";
			if (type == NUMBER)
				ss << std::get<int>(literal);
			else if (type == STRING)
				ss << std::get<std::string>(literal);
			else
				ss << "[none]";
			ss << ", str = \"" << str << "\", line = " << line << " }";
			return ss.str();
		};
	};

	enum ScanErrorType {
		SCAN_LINE_OK,
		SCAN_LINE_INT_OUT_OF_BOUNDS,
	};
	struct ScanError {
		ScanErrorType type;
		int line;
	};
	ScanError scan(const std::vector<std::string> &lines, std::vector<lex::Token> &tokens);
}

#endif

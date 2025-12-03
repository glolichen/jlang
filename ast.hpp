#ifndef AST_HPP
#define AST_HPP

#include <vector>
#include "lex.hpp"

namespace ast {
	enum NodeType {
		NODE_ROOT,
		NODE_EXPR,
		NODE_TERM,
		NODE_FACTOR,
		NODE_LEAF,
	};
	struct Node {
		NodeType type;

		// terminals are lex::Token, nonterminals are Node
		// i.e. if type is NODE_TERMINAL, then "children" is lex::Token
		// if not, then it is a vector of child nodes
		std::variant<std::vector<Node>, lex::Token> children_or_value;

		Node(NodeType type) {
			this->type = type;
		}
		Node() {
			this->type = NODE_ROOT;
		}

		// TODO: Safety -- if current node type is terminal, do not allow this
		// if adding node type is terminal, use insert_leaf instead
		Node &insert_node(NodeType type) {
			std::vector<Node> &children = std::get<std::vector<Node>>(children_or_value);
			children.emplace_back(Node(type));
			return children[children.size() - 1];
		}
		
		Node &insert_leaf(const lex::Token &t) {
			Node new_node = Node(NODE_LEAF);
			new_node.children_or_value = t;

			std::vector<Node> &children = std::get<std::vector<Node>>(children_or_value);
			children.emplace_back(new_node);
			return children[children.size() - 1];
		}
	};
}

#endif


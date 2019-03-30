
#pragma once

#include <memory>
#include <ostream>
#include <string>

namespace HDLCompiler {

namespace AST {
class BlockNode;
}

struct TypeSymbol {
	enum class Type {
		Block
	};

	std::string name;
	Type type;
	std::shared_ptr<AST::BlockNode> block;

	TypeSymbol(std::string name, Type type);
	TypeSymbol(std::string name, std::shared_ptr<AST::BlockNode> block);
};

std::ostream& operator<<(std::ostream &stream, const TypeSymbol &symbol);

}

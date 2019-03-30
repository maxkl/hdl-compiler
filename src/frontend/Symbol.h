
#pragma once

#include <cstdint>
#include <memory>
#include <ostream>
#include <string>

namespace HDLCompiler {

namespace AST {
class BlockNode;
}

struct Symbol {
	enum class Type {
		In,
		Out,
		Block
	};

	struct TypeData {
		Type type;
		std::uint64_t width;
		std::shared_ptr<AST::BlockNode> block;

		TypeData(Type type, std::uint64_t width);
		TypeData(std::shared_ptr<AST::BlockNode> block, std::uint64_t width);
		std::string to_string() const;
	};

	std::string name;
	std::shared_ptr<TypeData> type;
	std::uint32_t signal;

	Symbol(std::string name, std::shared_ptr<TypeData> type);
};

std::ostream& operator<<(std::ostream &stream, const Symbol &symbol);

}

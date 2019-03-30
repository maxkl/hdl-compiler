#include <utility>


#include "Symbol.h"

#include "AST.h"

namespace HDLCompiler {

Symbol::TypeData::TypeData(Symbol::Type type, std::uint64_t width)
	: type(type), width(width) {
	//
}

Symbol::TypeData::TypeData(std::shared_ptr<AST::BlockNode> block, std::uint64_t width)
	: type(Symbol::Type::Block), width(width), block(std::move(block)) {
	//
}

std::string Symbol::TypeData::to_string() const {
	std::string str;

	switch (type) {
		case Type::In:
		    str += "in";
		    break;
		case Type::Out:
		    str += "out";
		    break;
		case Type::Block:
		    str += "block " + block->identifier->value;
		    break;
	}

	if (width != 1) {
		str += "[" + std::to_string(width) + "]";
	}

	return str;
}

Symbol::Symbol(std::string name, std::shared_ptr<Symbol::TypeData> type)
	: name(std::move(name)), type(std::move(type)) {
	//
}

std::ostream& operator<<(std::ostream &stream, const Symbol &symbol) {
	stream << symbol.type->to_string() << " " << symbol.name << " @ " << symbol.signal;
	return stream;
}

}

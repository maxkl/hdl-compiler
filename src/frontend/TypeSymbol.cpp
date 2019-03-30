
#include "TypeSymbol.h"

#include <ostream>

namespace HDLCompiler {

TypeSymbol::TypeSymbol(std::string name, TypeSymbol::Type type)
	: name(std::move(name)), type(type) {
	//
}

TypeSymbol::TypeSymbol(std::string name, std::shared_ptr<AST::BlockNode> block)
	: name(std::move(name)), type(TypeSymbol::Type::Block), block(std::move(block)) {
	//
}

std::ostream& operator<<(std::ostream &stream, const TypeSymbol &symbol) {
	switch (symbol.type) {
		case TypeSymbol::Type::Block:
			stream << "block";
			break;
	}
	stream << " " << symbol.name;
	return stream;
}

}

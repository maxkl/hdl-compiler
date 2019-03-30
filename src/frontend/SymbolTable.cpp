
#include "SymbolTable.h"

#include <shared/errors.h>
#include <shared/helper/stacktrace.h>

namespace HDLCompiler {

SymbolTable::SymbolTable(std::shared_ptr<HDLCompiler::SymbolTable> parent)
	: parent(std::move(parent)) {
	//
}

bool SymbolTable::add(std::unique_ptr<Symbol> symbol) {
	auto ret = symbols.insert(std::make_pair(symbol->name, std::move(symbol)));
	bool success = ret.second;
	if (success) {
		orderedSymbols.push_back((*ret.first).second.get());
	}

	return success;
}

bool SymbolTable::addType(std::unique_ptr<TypeSymbol> symbol) {
	return types.insert(std::make_pair(symbol->name, std::move(symbol))).second;
}

Symbol *SymbolTable::find(const std::string &name) const {
	auto it = symbols.find(name);

	if (it == symbols.end()) {
		return nullptr;
	}

	return (*it).second.get();
}

Symbol *SymbolTable::findRecursive(const std::string &name) const {
	Symbol *symbol = find(name);

	if (symbol) {
		return symbol;
	}

	if (!parent) {
		return nullptr;
	}

	return parent->find(name);
}

TypeSymbol *SymbolTable::findType(const std::string &name) const {
	auto it = types.find(name);

	if (it == types.end()) {
		return nullptr;
	}

	return (*it).second.get();
}

TypeSymbol *SymbolTable::findTypeRecursive(const std::string &name) const {
	TypeSymbol *symbol = findType(name);

	if (symbol) {
		return symbol;
	}

	if (!parent) {
		return nullptr;
	}

	return parent->findType(name);
}

const std::vector<Symbol *> &SymbolTable::getSymbols() {
    return orderedSymbols;
}

}

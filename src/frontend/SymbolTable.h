
#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <ostream>
#include <vector>

#include "Symbol.h"
#include "TypeSymbol.h"

namespace HDLCompiler {

class SymbolTable {
public:
    explicit SymbolTable(std::shared_ptr<SymbolTable> parent = nullptr);

//	void print(std::ostream &stream) const;

    bool add(std::unique_ptr<Symbol> symbol);
    bool addType(std::unique_ptr<TypeSymbol> symbol);

    Symbol *find(const std::string &name) const;
    Symbol *findRecursive(const std::string &name) const;
    TypeSymbol *findType(const std::string &name) const;
    TypeSymbol *findTypeRecursive(const std::string &name) const;

    const std::vector<Symbol *> &getSymbols();

private:
    const std::shared_ptr<SymbolTable> parent;

    std::unordered_map<std::string, std::unique_ptr<Symbol>> symbols;
    std::vector<Symbol *> orderedSymbols;
    std::unordered_map<std::string, std::unique_ptr<TypeSymbol>> types;
};

}

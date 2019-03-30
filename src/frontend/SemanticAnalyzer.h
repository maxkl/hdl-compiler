
#pragma once

#include "AST.h"
#include "SymbolTable.h"

namespace HDLCompiler {

class SemanticAnalyzer {
public:
    static void analyze(AST::RootNode &ast);

private:
    static ExpressionResultType analyzeBinaryExpression(AST::BinaryExpressionNode &expression, SymbolTable &symbolTable);
    static ExpressionResultType analyzeUnaryExpression(AST::UnaryExpressionNode &expression, SymbolTable &symbolTable);
    static ExpressionResultType analyzeExpression(AST::ExpressionNode &expression, SymbolTable &symbolTable);
    static std::tuple<std::uint64_t, std::uint64_t> analyzeSubscript(AST::SubscriptNode &subscript, SymbolTable &symbolTable);
    static ExpressionResultType analyzeBehaviourIdentifier(AST::BehaviourIdentifierNode &behaviourIdentifier, SymbolTable &symbolTable);
    static void analyzeBehaviourStatement(AST::BehaviourStatementNode &behaviourStatement, SymbolTable &symbolTable);
    static void analyzeDeclarationIdentifier(AST::IdentifierNode &identifier, SymbolTable &symbolTable, std::shared_ptr<Symbol::TypeData> symbolType);
    static std::shared_ptr<Symbol::TypeData> analyzeType(AST::TypeNode &type, SymbolTable &symbolTable);
    static void analyzeDeclaration(AST::DeclarationNode &declaration, SymbolTable &symbolTable);
    static void analyzeBlock(std::shared_ptr<AST::BlockNode> block, std::shared_ptr<SymbolTable> &symbolTable);
};

}

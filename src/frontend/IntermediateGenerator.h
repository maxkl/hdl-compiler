
#pragma once

#include <shared/Intermediate.h>
#include <shared/IntermediateFile.h>

#include "AST.h"

namespace HDLCompiler {

struct IntermediateGenerator {
    static Intermediate::File generate(AST::RootNode &ast);

private:
    static std::vector<std::shared_ptr<Intermediate::Block>> generateBlocks(std::vector<std::shared_ptr<AST::BlockNode>> &blocks);
    static std::shared_ptr<Intermediate::Block> generateBlock(AST::BlockNode &block);
    static void generateBehaviourStatements(std::vector<std::unique_ptr<AST::BehaviourStatementNode>> &behaviourStatements, SymbolTable &symbolTable, Intermediate::Block &block);
    static void generateBehaviourStatement(AST::BehaviourStatementNode &behaviourStatement, SymbolTable &symbolTable, Intermediate::Block &block);
    static std::uint32_t generateBehaviourIdentifier(AST::BehaviourIdentifierNode &behaviourIdentifier, SymbolTable &symbolTable);
    static std::uint32_t generateExpression(AST::ExpressionNode &expression, SymbolTable &symbolTable, Intermediate::Block &block);
    static std::uint32_t generateConstant(std::uint64_t value, std::uint64_t width, Intermediate::Block &block);
    static std::uint32_t generateUnaryExpression(AST::UnaryExpressionNode &expression, SymbolTable &symbolTable, Intermediate::Block &block);
    static std::uint32_t generateBinaryExpression(AST::BinaryExpressionNode &expression, SymbolTable &symbolTable, Intermediate::Block &block);
};

}


#pragma once

#include <memory>

#include "Lexer.h"
#include "AST.h"

namespace HDLCompiler {

class Parser {
public:
    explicit Parser(Lexer &lexer);

    std::unique_ptr<AST::RootNode> parse();

private:
    Lexer &lexer;
    std::unique_ptr<Lexer::Token> lookahead;

    void readLookahead();
    std::unique_ptr<Lexer::Token> match(Lexer::TokenType type);

    std::unique_ptr<AST::IdentifierNode> parseIdentifier();
    std::unique_ptr<AST::NumberNode> parseNumber();
    std::unique_ptr<AST::ExpressionNode> parsePrimaryExpression();
    std::unique_ptr<AST::ExpressionNode> parseUnaryExpression();
    std::unique_ptr<AST::ExpressionNode> parseBitwiseAndExpression();
    std::unique_ptr<AST::ExpressionNode> parseBitwiseXorExpression();
    std::unique_ptr<AST::ExpressionNode> parseBitwiseOrExpression();
    std::unique_ptr<AST::ExpressionNode> parseExpression();
    std::unique_ptr<AST::SubscriptNode> parseSubscript();
    std::unique_ptr<AST::BehaviourIdentifierNode> parseBehaviourIdentifier();
    std::unique_ptr<AST::BehaviourStatementNode> parseBehaviourStatement();
    std::vector<std::unique_ptr<AST::BehaviourStatementNode>> parseBehaviourStatements();
    std::vector<std::unique_ptr<AST::IdentifierNode>> parseIdentifierList();
    std::unique_ptr<AST::TypeSpecifierNode> parseTypeSpecifier();
    std::unique_ptr<AST::TypeNode> parseType();
    std::unique_ptr<AST::DeclarationNode> parseDeclaration();
    std::vector<std::unique_ptr<AST::DeclarationNode>> parseDeclarations();
    std::shared_ptr<AST::BlockNode> parseBlock();
    std::vector<std::shared_ptr<AST::BlockNode>> parseBlocks();
};

}

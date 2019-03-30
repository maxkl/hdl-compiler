
#include "Parser.h"

#include <shared/errors.h>

namespace HDLCompiler {

Parser::Parser(Lexer &lexer)
    : lexer(lexer) {
    //
}

void Parser::readLookahead() {
    lookahead = std::make_unique<Lexer::Token>(lexer.readNextToken());
}

std::unique_ptr<Lexer::Token> Parser::match(Lexer::TokenType type) {
    if (lookahead->type != type) {
        throw CompilerError(lookahead->location.to_string() + ": Expected " + Lexer::TokenType_to_string(type) + " but got " + Lexer::TokenType_to_string(lookahead->type));
    }

    std::unique_ptr<Lexer::Token> token(std::move(lookahead));

    readLookahead();

    return token;
}

/*
 * blocks               -> ( block )*
 * block                -> 'block' identifier '{' declarations behaviour_statements '}'
 * declarations         -> ( declaration )*
 * declaration          -> type identifier_list ';'
 * identifier_list      -> identifier ( ',' identifier )*
 * type                 -> type_specifier [ '[' number ']' ]
 * type_specifier       -> 'in' | 'out' | 'block' identifier
 * behaviour_statements -> ( behaviour_statement )*
 * behaviour_statement  -> behaviour_identifier '=' expression ';'
 * behaviour_identifier -> identifier [ '.' identifier ] [ subscript ]
 * subscript            -> '[' number [ ':' number ] ']'
 */

/*
 * Operators:
 *
 * Unary:
 *   Bitwise NOT: ~
 * Bitwise AND: &
 * Bitwise XOR: ^
 * Bitwise OR: |
 */

/*
 * expr -> bit_or_expr
 * bit_or_expr -> bit_xor_expr ( '|' bit_xor_expr )*
 * bit_xor_expr -> bit_and_expr ( '^' bit_and_expr )*
 * bit_and_expr -> unary_expr ( '&' unary_expr )*
 * unary_expr -> '~' unary_expr | primary_expr
 * primary_expr -> '(' expr ')' | behaviour_identifier | number
 */

std::unique_ptr<AST::IdentifierNode> Parser::parseIdentifier() {
    std::shared_ptr<Lexer::Token> token = match(Lexer::TokenType::Identifier);

    return std::make_unique<AST::IdentifierNode>(token->identifier);
}

std::unique_ptr<AST::NumberNode> Parser::parseNumber() {
    std::shared_ptr<Lexer::Token> token = match(Lexer::TokenType::Number);

    return std::make_unique<AST::NumberNode>(token->number.value, token->number.width);
}

std::unique_ptr<AST::ExpressionNode> Parser::parsePrimaryExpression() {
    switch (lookahead->type) {
        case Lexer::TokenType::LeftParenthesis: {
            match(Lexer::TokenType::LeftParenthesis);

            std::unique_ptr<AST::ExpressionNode> expression = parseExpression();

            match(Lexer::TokenType::RightParenthesis);

            return expression;
        }
        case Lexer::TokenType::Identifier: {
            std::unique_ptr<AST::BehaviourIdentifierNode> identifier = parseBehaviourIdentifier();

            return std::make_unique<AST::VariableExpressionNode>(std::move(identifier));
        }
        case Lexer::TokenType::Number: {
            std::unique_ptr<AST::NumberNode> number = parseNumber();

            return std::make_unique<AST::ConstantExpressionNode>(std::move(number));
        }
        default:
            throw CompilerError(lookahead->location.to_string() + ": Expected '(', identifier or number");
    }
}

std::unique_ptr<AST::ExpressionNode> Parser::parseUnaryExpression() {
    switch (lookahead->type) {
        case Lexer::TokenType::NOT: {
            match(Lexer::TokenType::NOT);

            std::unique_ptr<AST::ExpressionNode> operand = parseUnaryExpression();

            return std::make_unique<AST::UnaryExpressionNode>(AST::UnaryOperator::NOT, std::move(operand));
        }
        default:
            return parsePrimaryExpression();
    }
}

std::unique_ptr<AST::ExpressionNode> Parser::parseBitwiseAndExpression() {
    std::unique_ptr<AST::ExpressionNode> left = parseUnaryExpression();

    while (lookahead->type == Lexer::TokenType::AND) {
        match(Lexer::TokenType::AND);

        std::unique_ptr<AST::ExpressionNode> right = parseUnaryExpression();

        left = std::make_unique<AST::BinaryExpressionNode>(AST::BinaryOperator::AND, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<AST::ExpressionNode> Parser::parseBitwiseXorExpression() {
    std::unique_ptr<AST::ExpressionNode> left = parseBitwiseAndExpression();

    while (lookahead->type == Lexer::TokenType::XOR) {
        match(Lexer::TokenType::XOR);

        std::unique_ptr<AST::ExpressionNode> right = parseBitwiseAndExpression();

        left = std::make_unique<AST::BinaryExpressionNode>(AST::BinaryOperator::XOR, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<AST::ExpressionNode> Parser::parseBitwiseOrExpression() {
    std::unique_ptr<AST::ExpressionNode> left = parseBitwiseXorExpression();

    while (lookahead->type == Lexer::TokenType::OR) {
        match(Lexer::TokenType::OR);

        std::unique_ptr<AST::ExpressionNode> right = parseBitwiseXorExpression();

        left = std::make_unique<AST::BinaryExpressionNode>(AST::BinaryOperator::OR, std::move(left), std::move(right));
    }

    return left;
}

std::unique_ptr<AST::ExpressionNode> Parser::parseExpression() {
    return parseBitwiseOrExpression();
}

std::unique_ptr<AST::SubscriptNode> Parser::parseSubscript() {
    match(Lexer::TokenType::LeftBracket);

    std::unique_ptr<AST::NumberNode> start = parseNumber();

    // Optional end index
    std::unique_ptr<AST::NumberNode> end;
    if (lookahead->type == Lexer::TokenType::Colon) {
        match(Lexer::TokenType::Colon);

        end = parseNumber();
    }

    match(Lexer::TokenType::RightBracket);

    return std::make_unique<AST::SubscriptNode>(std::move(start), std::move(end));
}

std::unique_ptr<AST::BehaviourIdentifierNode> Parser::parseBehaviourIdentifier() {
    std::unique_ptr<AST::IdentifierNode> identifier = parseIdentifier();

    // Optional property access
    std::unique_ptr<AST::IdentifierNode> propertyIdentifier;
    if (lookahead->type == Lexer::TokenType::Dot) {
        match(Lexer::TokenType::Dot);

        propertyIdentifier = parseIdentifier();
    }

    // Optional subscript
    std::unique_ptr<AST::SubscriptNode> subscript;
    if (lookahead->type == Lexer::TokenType::LeftBracket) {
        subscript = parseSubscript();
    }

    return std::make_unique<AST::BehaviourIdentifierNode>(std::move(identifier), std::move(propertyIdentifier), std::move(subscript));
}

std::unique_ptr<AST::BehaviourStatementNode> Parser::parseBehaviourStatement() {
    std::unique_ptr<AST::BehaviourIdentifierNode> behaviourIdentifier = parseBehaviourIdentifier();

    match(Lexer::TokenType::Equals);

    std::unique_ptr<AST::ExpressionNode> expression = parseExpression();

    match(Lexer::TokenType::Semicolon);

    return std::make_unique<AST::BehaviourStatementNode>(std::move(behaviourIdentifier), std::move(expression));
}

std::vector<std::unique_ptr<AST::BehaviourStatementNode>> Parser::parseBehaviourStatements() {
    std::vector<std::unique_ptr<AST::BehaviourStatementNode>> behaviourStatements;

    while (lookahead->type == Lexer::TokenType::Identifier) {
        behaviourStatements.push_back(parseBehaviourStatement());
    }

    return behaviourStatements;
}

std::vector<std::unique_ptr<AST::IdentifierNode>> Parser::parseIdentifierList() {
    std::vector<std::unique_ptr<AST::IdentifierNode>> identifierList;

    identifierList.push_back(parseIdentifier());

    while (lookahead->type == Lexer::TokenType::Comma) {
        match(Lexer::TokenType::Comma);

        identifierList.push_back(parseIdentifier());
    }

    return identifierList;
}

std::unique_ptr<AST::TypeSpecifierNode> Parser::parseTypeSpecifier() {
    switch (lookahead->type) {
        case Lexer::TokenType::InKeyword:
            match(Lexer::TokenType::InKeyword);

            return std::make_unique<AST::TypeSpecifierNode>(AST::TypeSpecifierType::In);
        case Lexer::TokenType::OutKeyword:
            match(Lexer::TokenType::OutKeyword);

            return std::make_unique<AST::TypeSpecifierNode>(AST::TypeSpecifierType::Out);
        case Lexer::TokenType::BlockKeyword: {
            match(Lexer::TokenType::BlockKeyword);

            std::unique_ptr<AST::IdentifierNode> identifier = parseIdentifier();

            return std::make_unique<AST::TypeSpecifierBlockNode>(std::move(identifier));
        }
        default:
            throw CompilerError(lookahead->location.to_string() + ": Expected type");
    }
}

std::unique_ptr<AST::TypeNode> Parser::parseType() {
    std::unique_ptr<AST::TypeSpecifierNode> typeSpecifier = parseTypeSpecifier();

    // Optional type width
    std::unique_ptr<AST::NumberNode> width;
    if (lookahead->type == Lexer::TokenType::LeftBracket) {
        match(Lexer::TokenType::LeftBracket);

        width = parseNumber();

        match(Lexer::TokenType::RightBracket);
    }

    return std::make_unique<AST::TypeNode>(std::move(typeSpecifier), std::move(width));
}

std::unique_ptr<AST::DeclarationNode> Parser::parseDeclaration() {
    std::unique_ptr<AST::TypeNode> type = parseType();

    std::vector<std::unique_ptr<AST::IdentifierNode>> identifierList = parseIdentifierList();

    match(Lexer::TokenType::Semicolon);

    return std::make_unique<AST::DeclarationNode>(std::move(type), std::move(identifierList));
}

std::vector<std::unique_ptr<AST::DeclarationNode>> Parser::parseDeclarations() {
    std::vector<std::unique_ptr<AST::DeclarationNode>> declarations;

    while (lookahead->type == Lexer::TokenType::InKeyword
        || lookahead->type == Lexer::TokenType::OutKeyword
        || lookahead->type == Lexer::TokenType::BlockKeyword
        ) {
        declarations.push_back(parseDeclaration());
    }

    return declarations;
}

std::shared_ptr<AST::BlockNode> Parser::parseBlock() {
    match(Lexer::TokenType::BlockKeyword);

    std::unique_ptr<AST::IdentifierNode> identifier = parseIdentifier();

    match(Lexer::TokenType::LeftBrace);

    std::vector<std::unique_ptr<AST::DeclarationNode>> declarations = parseDeclarations();

    std::vector<std::unique_ptr<AST::BehaviourStatementNode>> behaviourStatements = parseBehaviourStatements();

    match(Lexer::TokenType::RightBrace);

    return std::make_shared<AST::BlockNode>(std::move(identifier), std::move(declarations), std::move(behaviourStatements));
}

std::vector<std::shared_ptr<AST::BlockNode>> Parser::parseBlocks() {
    std::vector<std::shared_ptr<AST::BlockNode>> blocks;

    while (lookahead->type == Lexer::TokenType::BlockKeyword) {
        blocks.push_back(parseBlock());
    }

    return blocks;
}

std::unique_ptr<AST::RootNode> Parser::parse() {
    readLookahead();

    std::unique_ptr<AST::RootNode> root = std::make_unique<AST::RootNode>(parseBlocks());

    if (lookahead->type != Lexer::TokenType::EndOfFile) {
        throw CompilerError(lookahead->location.to_string() + ": Unexpected " + Lexer::TokenType_to_string(lookahead->type) + ", expected block");
    }

    return root;
}

}

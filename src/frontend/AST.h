
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <ostream>

#include "ExpressionResultType.h"
#include "SymbolTable.h"
#include <shared/Intermediate.h>

namespace HDLCompiler {

namespace AST {

struct Node {
public:
    virtual void print(std::ostream &stream, unsigned int indentationLevel) const = 0;
    void print(std::ostream &stream) const;

    static void print(const Node *node, std::ostream &stream, unsigned int indentationLevel = 0);

protected:
    Node() = default;
};

struct IdentifierNode : public Node {
    std::string value;

    explicit IdentifierNode(std::string value);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

struct NumberNode : public Node {
    std::uint64_t value;
    std::uint64_t width;

    NumberNode(std::uint64_t value, std::uint64_t width);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

struct SubscriptNode : public Node {
    std::unique_ptr<NumberNode> start;
    std::unique_ptr<NumberNode> end;

    std::uint64_t startIndex;
    std::uint64_t endIndex;

    SubscriptNode(std::unique_ptr<NumberNode> start, std::unique_ptr<NumberNode> end);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

struct BehaviourIdentifierNode : public Node {
    std::unique_ptr<IdentifierNode> identifier;
    std::unique_ptr<IdentifierNode> propertyIdentifier;
    std::unique_ptr<SubscriptNode> subscript;

    BehaviourIdentifierNode(std::unique_ptr<IdentifierNode> identifier, std::unique_ptr<IdentifierNode> propertyIdentifier, std::unique_ptr<SubscriptNode> subscript);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

enum class ExpressionType {
    Binary,
    Unary,
    Variable,
    Constant
};

struct ExpressionNode : public Node {
    ExpressionType type;

    ExpressionResultType resultType;

protected:
    explicit ExpressionNode(ExpressionType type);
};

struct ConstantExpressionNode : public ExpressionNode {
    std::unique_ptr<NumberNode> number;

    explicit ConstantExpressionNode(std::unique_ptr<NumberNode> number);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

struct VariableExpressionNode : public ExpressionNode {
    std::unique_ptr<BehaviourIdentifierNode> identifier;

    explicit VariableExpressionNode(std::unique_ptr<BehaviourIdentifierNode> identifier);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

enum class UnaryOperator {
    NOT
};

struct UnaryExpressionNode : public ExpressionNode {
    UnaryOperator op;
    std::unique_ptr<ExpressionNode> operand;

    UnaryExpressionNode(UnaryOperator op, std::unique_ptr<ExpressionNode> operand);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

enum class BinaryOperator {
    AND,
    OR,
    XOR
};

struct BinaryExpressionNode : public ExpressionNode {
    BinaryOperator op;
    std::unique_ptr<ExpressionNode> leftOperand;
    std::unique_ptr<ExpressionNode> rightOperand;

    BinaryExpressionNode(BinaryOperator op, std::unique_ptr<ExpressionNode> leftOperand, std::unique_ptr<ExpressionNode> rightOperand);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

struct BehaviourStatementNode : public Node {
    std::unique_ptr<BehaviourIdentifierNode> behaviourIdentifier;
    std::unique_ptr<ExpressionNode> expression;

    ExpressionResultType resultType;

    BehaviourStatementNode(std::unique_ptr<BehaviourIdentifierNode> behaviourIdentifier, std::unique_ptr<ExpressionNode> expression);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

enum class TypeSpecifierType {
    In,
    Out,
    Block
};

struct TypeSpecifierNode : public Node {
    TypeSpecifierType type;

    explicit TypeSpecifierNode(TypeSpecifierType type);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

struct TypeSpecifierBlockNode : public TypeSpecifierNode {
    std::unique_ptr<IdentifierNode> identifier;

    explicit TypeSpecifierBlockNode(std::unique_ptr<IdentifierNode> identifier);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

struct TypeNode : public Node {
    std::unique_ptr<TypeSpecifierNode> typeSpecifier;
    std::unique_ptr<NumberNode> width;

    TypeNode(std::unique_ptr<TypeSpecifierNode> typeSpecifier, std::unique_ptr<NumberNode> width);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

struct DeclarationNode : public Node {
    std::unique_ptr<TypeNode> type;
    std::vector<std::unique_ptr<IdentifierNode>> identifiers;

    DeclarationNode(std::unique_ptr<TypeNode> type, std::vector<std::unique_ptr<IdentifierNode>> identifiers);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

struct BlockNode : public Node {
    std::unique_ptr<IdentifierNode> identifier;
    std::vector<std::unique_ptr<DeclarationNode>> declarations;
    std::vector<std::unique_ptr<BehaviourStatementNode>> behaviourStatements;

    std::shared_ptr<SymbolTable> symbolTable;

    std::shared_ptr<Intermediate::Block> intermediateBlock;

    BlockNode(std::unique_ptr<IdentifierNode> identifier, std::vector<std::unique_ptr<DeclarationNode>> declarations, std::vector<std::unique_ptr<BehaviourStatementNode>> behaviourStatements);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

struct RootNode : public Node {
    std::vector<std::shared_ptr<AST::BlockNode>> blocks;

    std::shared_ptr<SymbolTable> symbolTable;

    explicit RootNode(std::vector<std::shared_ptr<AST::BlockNode>> blocks);

    void print(std::ostream &stream, unsigned int indentationLevel) const override;
};

}

}

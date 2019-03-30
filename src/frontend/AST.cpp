
#include "AST.h"

namespace HDLCompiler {

namespace AST {

void Node::print(std::ostream &stream) const {
    print(stream, 0);
}

void Node::print(const Node *node, std::ostream &stream, unsigned int indentationLevel) {
    if (node) {
        node->print(stream, indentationLevel);
    } else {
        stream << std::string(indentationLevel * 2, ' ') << "(null)";
    }
}

IdentifierNode::IdentifierNode(std::string value)
    : value(std::move(value)) {
    //
}

void IdentifierNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    stream << std::string(indentationLevel * 2, ' ') << "Identifier: \"" << value << "\"\n";
}

NumberNode::NumberNode(std::uint64_t value, std::uint64_t width)
    : value(value), width(width) {
    //
}

void NumberNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    stream << std::string(indentationLevel * 2, ' ') << "Number: " << value << "#" << width << "\n";
}

SubscriptNode::SubscriptNode(std::unique_ptr<NumberNode> start, std::unique_ptr<NumberNode> end)
    : start(std::move(start)), end(std::move(end)), startIndex(0), endIndex(0) {
    //
}

void SubscriptNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    stream << std::string(indentationLevel * 2, ' ') << "Subscript\n";
    Node::print(start.get(), stream, indentationLevel + 1);
    Node::print(end.get(), stream, indentationLevel + 1);
}

BehaviourIdentifierNode::BehaviourIdentifierNode(std::unique_ptr<IdentifierNode> identifier, std::unique_ptr<IdentifierNode> propertyIdentifier, std::unique_ptr<SubscriptNode> subscript)
    : identifier(std::move(identifier)), propertyIdentifier(std::move(propertyIdentifier)), subscript(std::move(subscript)) {
    //
}

void BehaviourIdentifierNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    stream << std::string(indentationLevel * 2, ' ') << "BehaviourIdentifier\n";
    Node::print(identifier.get(), stream, indentationLevel + 1);
    Node::print(propertyIdentifier.get(), stream, indentationLevel + 1);
    Node::print(subscript.get(), stream, indentationLevel + 1);
}

ExpressionNode::ExpressionNode(ExpressionType type)
    : type(type) {
    //
}

ConstantExpressionNode::ConstantExpressionNode(std::unique_ptr<NumberNode> number)
    : ExpressionNode(ExpressionType::Constant), number(std::move(number)) {
    //
}

void ConstantExpressionNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    stream << std::string(indentationLevel * 2, ' ') << "ConstantExpression\n";
    Node::print(number.get(), stream, indentationLevel + 1);
}

VariableExpressionNode::VariableExpressionNode(std::unique_ptr<BehaviourIdentifierNode> identifier)
    : ExpressionNode(ExpressionType::Variable), identifier(std::move(identifier)) {
    //
}

void VariableExpressionNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    stream << std::string(indentationLevel * 2, ' ') << "VariableExpression\n";
    Node::print(identifier.get(), stream, indentationLevel + 1);
}

UnaryExpressionNode::UnaryExpressionNode(UnaryOperator op, std::unique_ptr<ExpressionNode> operand)
    : ExpressionNode(ExpressionType::Unary), op(op), operand(std::move(operand)) {
    //
}

void UnaryExpressionNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    stream << std::string(indentationLevel * 2, ' ') << "UnaryExpression: ";
    switch (op) {
        case UnaryOperator::NOT: stream << "NOT";
    }
    stream << "\n";
    Node::print(operand.get(), stream, indentationLevel + 1);
}

BinaryExpressionNode::BinaryExpressionNode(BinaryOperator op, std::unique_ptr<ExpressionNode> leftOperand, std::unique_ptr<ExpressionNode> rightOperand)
    : ExpressionNode(ExpressionType::Binary), op(op), leftOperand(std::move(leftOperand)), rightOperand(std::move(rightOperand)) {
    //
}

void BinaryExpressionNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    stream << std::string(indentationLevel * 2, ' ') << "BinaryExpression: ";
    switch (op) {
        case BinaryOperator::AND:
            stream << "AND";
            break;
        case BinaryOperator::OR:
            stream << "OR";
            break;
        case BinaryOperator::XOR:
            stream << "XOR";
            break;
    }
    stream << "\n";
    Node::print(leftOperand.get(), stream, indentationLevel + 1);
    Node::print(rightOperand.get(), stream, indentationLevel + 1);
}

BehaviourStatementNode::BehaviourStatementNode(std::unique_ptr<BehaviourIdentifierNode> behaviourIdentifier, std::unique_ptr<ExpressionNode> expression)
    : behaviourIdentifier(std::move(behaviourIdentifier)), expression(std::move(expression)) {
    //
}

void BehaviourStatementNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    stream << std::string(indentationLevel * 2, ' ') << "BehaviourStatement\n";
    Node::print(behaviourIdentifier.get(), stream, indentationLevel + 1);
    Node::print(expression.get(), stream, indentationLevel + 1);
}

TypeSpecifierNode::TypeSpecifierNode(TypeSpecifierType type)
    : type(type) {
    //
}

void TypeSpecifierNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    stream << std::string(indentationLevel * 2, ' ') << "TypeSpecifier: ";
    switch (type) {
        case TypeSpecifierType::In:
            stream << "In";
            break;
        case TypeSpecifierType::Out:
            stream << "Out";
            break;
        case TypeSpecifierType::Block:
            stream << "Block";
            break;
    }
    stream << "\n";
}

TypeSpecifierBlockNode::TypeSpecifierBlockNode(std::unique_ptr<IdentifierNode> identifier)
    : TypeSpecifierNode(TypeSpecifierType::Block), identifier(std::move(identifier)) {
    //
}

void TypeSpecifierBlockNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    TypeSpecifierNode::print(stream, indentationLevel);
    Node::print(identifier.get(), stream, indentationLevel + 1);
}

TypeNode::TypeNode(std::unique_ptr<TypeSpecifierNode> typeSpecifier, std::unique_ptr<NumberNode> width)
    : typeSpecifier(std::move(typeSpecifier)), width(std::move(width)) {
    //
}

void TypeNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    stream << std::string(indentationLevel * 2, ' ') << "Type\n";
    Node::print(typeSpecifier.get(), stream, indentationLevel + 1);
    Node::print(width.get(), stream, indentationLevel + 1);
}

DeclarationNode::DeclarationNode(std::unique_ptr<TypeNode> type, std::vector<std::unique_ptr<IdentifierNode>> identifiers)
    : type(std::move(type)), identifiers(std::move(identifiers)) {
    //
}

void DeclarationNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    stream << std::string(indentationLevel * 2, ' ') << "Declaration\n";
    Node::print(type.get(), stream, indentationLevel + 1);
    for (const auto &identifier : identifiers) {
        Node::print(identifier.get(), stream, indentationLevel + 1);
    }
}

BlockNode::BlockNode(std::unique_ptr<IdentifierNode> identifier, std::vector<std::unique_ptr<DeclarationNode>> declarations, std::vector<std::unique_ptr<BehaviourStatementNode>> behaviourStatements)
    : identifier(std::move(identifier)), declarations(std::move(declarations)), behaviourStatements(std::move(behaviourStatements)) {
    //
}

void BlockNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    stream << std::string(indentationLevel * 2, ' ') << "Block: (symbol table)\n";
    Node::print(identifier.get(), stream, indentationLevel + 1);
    for (const auto &declaration : declarations) {
        Node::print(declaration.get(), stream, indentationLevel + 1);
    }
    for (const auto &behaviourStatement : behaviourStatements) {
        Node::print(behaviourStatement.get(), stream, indentationLevel + 1);
    }
}

RootNode::RootNode(std::vector<std::shared_ptr<AST::BlockNode>> blocks)
        : blocks(std::move(blocks)) {
    //
}

void RootNode::print(std::ostream &stream, unsigned int indentationLevel) const {
    stream << std::string(indentationLevel * 2, ' ') << "Root: (symbol table)\n";

    for (const auto &block : blocks) {
        Node::print(block.get(), stream, indentationLevel + 1);
    }
}
}

}

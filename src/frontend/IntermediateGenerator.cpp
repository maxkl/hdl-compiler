
#include "IntermediateGenerator.h"

#include "AST.h"
#include "SymbolTable.h"
#include "ExpressionResultType.h"

namespace HDLCompiler {

std::uint32_t IntermediateGenerator::generateBinaryExpression(AST::BinaryExpressionNode &expression, SymbolTable &symbolTable, Intermediate::Block &block) {
    ExpressionResultType &resultType = expression.resultType;

    std::uint32_t signalA = generateExpression(*expression.leftOperand, symbolTable, block);

    std::uint32_t signalB = generateExpression(*expression.rightOperand, symbolTable, block);

    std::uint32_t outputSignal = block.allocateSignals(resultType.width);

    switch (expression.op) {
        case AST::BinaryOperator::AND:
            for (std::uint32_t i = 0; i < resultType.width; i++) {
                Intermediate::Statement statement(Intermediate::Operation::AND, 2);
                statement.setInput(0, signalA + i);
                statement.setInput(1, signalB + i);
                statement.setOutput(0, outputSignal + i);
                block.addStatement(std::move(statement));
            }
            break;
        case AST::BinaryOperator::OR:
            for (std::uint32_t i = 0; i < resultType.width; i++) {
                Intermediate::Statement statement(Intermediate::Operation::OR, 2);
                statement.setInput(0, signalA + i);
                statement.setInput(1, signalB + i);
                statement.setOutput(0, outputSignal + i);
                block.addStatement(std::move(statement));
            }
            break;
        case AST::BinaryOperator::XOR:
            for (std::uint32_t i = 0; i < resultType.width; i++) {
                Intermediate::Statement statement(Intermediate::Operation::XOR, 2);
                statement.setInput(0, signalA + i);
                statement.setInput(1, signalB + i);
                statement.setOutput(0, outputSignal + i);
                block.addStatement(std::move(statement));
            }
            break;
        default:
            throw std::domain_error("AST BinaryExpression node has invalid op");
    }

    return outputSignal;
}

std::uint32_t IntermediateGenerator::generateUnaryExpression(AST::UnaryExpressionNode &expression, SymbolTable &symbolTable, Intermediate::Block &block) {
    ExpressionResultType &resultType = expression.resultType;

    std::uint32_t signal = generateExpression(*expression.operand, symbolTable, block);

    std::uint32_t outputSignal = block.allocateSignals(resultType.width);

    switch (expression.op) {
        case AST::UnaryOperator::NOT:
            for (std::uint32_t i = 0; i < resultType.width; i++) {
                Intermediate::Statement statement(Intermediate::Operation::NOT, 1);
                statement.setInput(0, signal + i);
                statement.setOutput(0, outputSignal + i);
                block.addStatement(statement);
            }
            break;
        default:
            throw std::domain_error("AST UnaryExpression node has invalid op");
    }

    return outputSignal;
}

std::uint32_t IntermediateGenerator::generateConstant(std::uint64_t value, std::uint64_t width, Intermediate::Block &block) {
    std::uint32_t signal = block.allocateSignals(width);

    for (std::uint32_t i = 0; i < width; i++) {
        bool bitValue = (value & (1u << i)) != 0;
        Intermediate::Statement statement(bitValue ? Intermediate::Operation::Const1 : Intermediate::Operation::Const0, 1);
        statement.setOutput(0, signal + i);
        block.addStatement(statement);
    }

    return signal;
}

std::uint32_t IntermediateGenerator::generateExpression(AST::ExpressionNode &expression, SymbolTable &symbolTable, Intermediate::Block &block) {
    switch (expression.type) {
        case AST::ExpressionType::Binary:
            return generateBinaryExpression(dynamic_cast<AST::BinaryExpressionNode &>(expression), symbolTable, block);
        case AST::ExpressionType::Unary:
            return generateUnaryExpression(dynamic_cast<AST::UnaryExpressionNode &>(expression), symbolTable, block);
        case AST::ExpressionType::Variable:
            return generateBehaviourIdentifier(*dynamic_cast<AST::VariableExpressionNode &>(expression).identifier, symbolTable);
        case AST::ExpressionType::Constant: {
            AST::NumberNode &number = *dynamic_cast<AST::ConstantExpressionNode &>(expression).number;
            return generateConstant(number.value, number.width, block);
        }
        default:
            throw std::domain_error("AST Expression node has invalid type");
    }
}

std::uint32_t IntermediateGenerator::generateBehaviourIdentifier(AST::BehaviourIdentifierNode &behaviourIdentifier, SymbolTable &symbolTable) {
    AST::IdentifierNode &identifier = *behaviourIdentifier.identifier;

    Symbol *symbol = symbolTable.findRecursive(identifier.value);

    std::uint32_t signal = symbol->signal;

    AST::IdentifierNode *propertyIdentifier = behaviourIdentifier.propertyIdentifier.get();
    if (propertyIdentifier) {
        SymbolTable &propertySymbolTable = *symbol->type->block->symbolTable;
        Symbol *propertySymbol = propertySymbolTable.find(propertyIdentifier->value);

        signal += propertySymbol->signal;
    }

    AST::SubscriptNode *subscript = behaviourIdentifier.subscript.get();
    if (subscript) {
        signal += subscript->endIndex;
    }

    return signal;
}

void IntermediateGenerator::generateBehaviourStatement(AST::BehaviourStatementNode &behaviourStatement, SymbolTable &symbolTable, Intermediate::Block &block) {
    ExpressionResultType &expressionResultType = behaviourStatement.resultType;
    std::uint64_t expressionWidth = expressionResultType.width;

    std::uint32_t targetSignal = generateBehaviourIdentifier(*behaviourStatement.behaviourIdentifier, symbolTable);

    std::uint32_t sourceSignal = generateExpression(*behaviourStatement.expression, symbolTable, block);

    for (std::uint32_t i = 0; i < expressionWidth; i++) {
        Intermediate::Statement statement(Intermediate::Operation::Connect, 1);
        statement.setInput(0, sourceSignal + i);
        statement.setOutput(0, targetSignal + i);
        block.addStatement(statement);
    }
}

void IntermediateGenerator::generateBehaviourStatements(std::vector<std::unique_ptr<AST::BehaviourStatementNode>> &behaviourStatements, SymbolTable &symbolTable, Intermediate::Block &block) {
    for (const auto &behaviourStatement : behaviourStatements) {
        generateBehaviourStatement(*behaviourStatement, symbolTable, block);
    }
}

std::shared_ptr<Intermediate::Block> IntermediateGenerator::generateBlock(AST::BlockNode &block) {
    SymbolTable &symbolTable = *block.symbolTable;

    const std::vector<Symbol *> &symbols = symbolTable.getSymbols();

    std::shared_ptr<Intermediate::Block> intermediateBlock = std::make_shared<Intermediate::Block>(block.identifier->value);

    for (const auto &symbol : symbols) {
        Symbol::TypeData *symbolType = symbol->type.get();

        if (symbolType->type == Symbol::Type::In) {
            symbol->signal = intermediateBlock->allocateInputSignals(symbolType->width);
        }
    }

    for (const auto &symbol : symbols) {
        Symbol::TypeData *symbolType = symbol->type.get();

        if (symbolType->type == Symbol::Type::Out) {
            symbol->signal = intermediateBlock->allocateOutputSignals(symbolType->width);
        }
    }

    for (const auto &symbol : symbols) {
        Symbol::TypeData *symbolType = symbol->type.get();

        if (symbolType->type == Symbol::Type::Block) {
            symbol->signal = intermediateBlock->addBlock(symbolType->block->intermediateBlock);
        }
    }

    generateBehaviourStatements(block.behaviourStatements, symbolTable, *intermediateBlock);

    block.intermediateBlock = intermediateBlock;

    return intermediateBlock;
}

std::vector<std::shared_ptr<Intermediate::Block>> IntermediateGenerator::generateBlocks(std::vector<std::shared_ptr<AST::BlockNode>> &blocks) {
    std::vector<std::shared_ptr<Intermediate::Block>> intermediateBlocks;

    intermediateBlocks.reserve(blocks.size());
    for (const auto &block : blocks) {
        intermediateBlocks.push_back(generateBlock(*block));
    }

    return intermediateBlocks;
}

Intermediate::File IntermediateGenerator::generate(AST::RootNode &ast) {
    return Intermediate::File(generateBlocks(ast.blocks));
}

}

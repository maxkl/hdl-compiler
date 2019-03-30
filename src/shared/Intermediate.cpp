
#include "Intermediate.h"

#include <iostream>

#include <shared/helper/string_format.h>
#include <shared/errors.h>

namespace HDLCompiler {

namespace Intermediate {

Block::Block(std::string name)
    : name(std::move(name)), inputSignals(0), outputSignals(0), nextSignal(0) {
    //
}

Block::Block(std::string name, std::uint32_t inputSignals, std::uint32_t outputSignals, std::vector<std::shared_ptr<Block>> blocks, std::vector<Statement> statements)
    : name(std::move(name)), inputSignals(inputSignals), outputSignals(outputSignals), blocks(std::move(blocks)), statements(std::move(statements)), nextSignal(inputSignals + outputSignals) {
    //
}

std::uint32_t Block::allocateSignals(std::uint32_t count) {
    std::uint32_t baseSignal = nextSignal;
    nextSignal += count;
    return baseSignal;
}

std::uint32_t Block::allocateInputSignals(std::uint32_t count) {
    if (outputSignals > 0 || !blocks.empty() || !statements.empty()) {
        throw CompilerError("Can't add input signals after adding output signals, blocks or statements");
    }

    std::uint32_t baseSignal = nextSignal;
    inputSignals += count;
    nextSignal += count;

    return baseSignal;
}

std::uint32_t Block::allocateOutputSignals(std::uint32_t count) {
    if (!blocks.empty() || !statements.empty()) {
        throw CompilerError("Can't add output signals after adding blocks or statements");
    }

    std::uint32_t baseSignal = nextSignal;
    outputSignals += count;
    nextSignal += count;

    return baseSignal;
}

std::uint32_t Block::addBlock(const std::shared_ptr<Block> &newBlock) {
    if (!statements.empty()) {
        throw CompilerError("Can't add block after adding statements");
    }

    blocks.push_back(newBlock);

    std::uint32_t baseSignal = nextSignal;
    nextSignal += newBlock->inputSignals + newBlock->outputSignals;

    return baseSignal;
}

void Block::addStatement(Statement statement) {
    statements.push_back(std::move(statement));
}

void Block::print(std::ostream &f) const {
    f << string_format("Block: name=\"%s\", inputs=%u, outputs=%u, blocks=%u, statements=%u\n", name, inputSignals, outputSignals, blocks.size(), statements.size());

    std::uint32_t signal = inputSignals + outputSignals;

    for (const auto &block : blocks) {
        f << string_format("Block: name=\"%s\", inputs=%u, outputs=%u, id=%u\n", block->name, block->inputSignals, block->outputSignals, signal);

        signal += block->inputSignals + block->outputSignals;
    }

    for (const auto &statement : statements) {
        f << "  Statement: op=";

        switch (statement.op) {
            case Operation::Connect:
                f << "Connect";
                break;
            case Operation::Const0:
                f << "Const0";
                break;
            case Operation::Const1:
                f << "Const1";
                break;
            case Operation::AND:
                f << "AND";
                break;
            case Operation::OR:
                f << "OR";
                break;
            case Operation::XOR:
                f << "XOR";
                break;
            case Operation::NOT:
                f << "NOT";
                break;
            case Operation::MUX:
                f << "MUX";
                break;
            default:
                f << "(invalid)";
        }

        f << string_format(", inputs=%u, outputs=%u\n", statement.inputs.size(), statement.outputs.size());

        for (const auto &inputSignal : statement.inputs) {
            f << string_format("    Input: id=%u\n", inputSignal);
        }

        for (const auto &outputSignal : statement.outputs) {
            f << string_format("    Output: id=%u\n", outputSignal);
        }
    }
}

Statement::Statement(HDLCompiler::Intermediate::Operation op, std::uint16_t size)
    : op(op), size(size) {
    std::size_t inputCount, outputCount;
    switch (op) {
        case Operation::Connect:
            if (size != 1) {
                throw CompilerError("CONNECT statements with size other than 1 not supported");
            }
            inputCount = 1;
            outputCount = 1;
            break;
        case Operation::Const0:
        case Operation::Const1:
            inputCount = 0;
            outputCount = 1;
            break;
        case Operation::AND:
        case Operation::OR:
        case Operation::XOR:
            inputCount = size;
            outputCount = 1;
            break;
        case Operation::NOT:
            if (size != 1) {
                throw CompilerError("NOT statements with size other than 1 not supported");
            }
            inputCount = 1;
            outputCount = 1;
            break;
        case Operation::MUX:
            // 'size' represents number of address lines
            // inputCount = number of address lines + number of data lines
            inputCount = size + (1u << size);
            outputCount = 1;
            break;
        default:
            throw CompilerError("Invalid intermediate statement operator");
    }
    inputs.resize(inputCount);
    outputs.resize(outputCount);
}

void Statement::setInput(std::uint32_t index, std::uint32_t signal) {
    inputs.at(index) = signal;
}

void Statement::setOutput(std::uint32_t index, std::uint32_t signal) {
    outputs.at(index) = signal;
}

}

}

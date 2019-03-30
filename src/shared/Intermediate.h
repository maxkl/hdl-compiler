
#pragma once

#include <string>
#include <vector>
#include <memory>

namespace HDLCompiler {

namespace Intermediate {

enum class Operation {
    Connect = 1,

    Const0,
    Const1,

    AND,
    OR,
    XOR,

    NOT,

    MUX
};

struct Statement {
    Operation op;
    std::uint16_t size;
    std::vector<std::uint32_t> inputs;
    std::vector<std::uint32_t> outputs;

    Statement(Operation op, std::uint16_t size);
    void setInput(std::uint32_t index, std::uint32_t signal);
    void setOutput(std::uint32_t index, std::uint32_t signal);
};

struct Block {
    std::string name;

    std::uint32_t inputSignals;
    std::uint32_t outputSignals;
    std::vector<std::shared_ptr<Block>> blocks;
    std::vector<Statement> statements;
    std::uint32_t nextSignal;

    explicit Block(std::string name);
    Block(std::string name, std::uint32_t inputSignals, std::uint32_t outputSignals, std::vector<std::shared_ptr<Block>> blocks, std::vector<Statement> statements);

    std::uint32_t allocateSignals(std::uint32_t count);
    std::uint32_t allocateInputSignals(std::uint32_t count);
    std::uint32_t allocateOutputSignals(std::uint32_t count);

    // TODO: non-owning pointer instead?
    std::uint32_t addBlock(const std::shared_ptr<Block> &block);

    void addStatement(Statement statement);

    void print(std::ostream &f) const;
};

}

}

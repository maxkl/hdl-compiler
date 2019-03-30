
#pragma once

#include <shared/IntermediateFile.h>

namespace HDLCompiler {

namespace Backend {

class LogicSimulator {
public:
    static void run(std::string outputFilename, const Intermediate::File &intermediateFile, const std::vector<std::string> &argv);

private:
    struct Options {
        bool ioComponents;
    };

    enum class ComponentType {
        Connect,
        Const0,
        Const1,
        AND,
        OR,
        XOR,
        NOT
    };

    struct Component {
        ComponentType type;
        std::vector<std::uint32_t> inputs;
        std::vector<std::uint32_t> outputs;

        explicit Component(ComponentType type);

        void addInput(std::uint32_t signal);
        void addOutput(std::uint32_t signal);
    };

    struct Circuit {
        std::uint32_t inputSignalCount;
        std::uint32_t outputSignalCount;
        std::uint32_t signalCount;
        std::vector<Component> components;

        Circuit(std::uint32_t inputSignalCount, std::uint32_t outputSignalCount);

        std::uint32_t allocateSignals(std::uint32_t count);
        void addComponent(Component component);
    };

    static void generateCircuit(std::ofstream &f, const Intermediate::File &intermediateFile, const Options &options);
    static void generateBlock(const Intermediate::Block &block, Circuit &circuit, const Options &options);
    static void outputCircuit(std::ofstream &f, const Circuit &circuit, const Options &options);
};

}

}

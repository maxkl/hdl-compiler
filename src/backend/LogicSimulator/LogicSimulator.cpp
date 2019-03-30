
#include "LogicSimulator.h"

#include <fstream>
#include <cstring>

#include <nlohmann/json.hpp>

#include <shared/Intermediate.h>
#include <shared/errors.h>

using json = nlohmann::json;

namespace HDLCompiler {

namespace Backend {

LogicSimulator::Component::Component(ComponentType type)
	: type(type) {
	//
}

void LogicSimulator::Component::addInput(std::uint32_t signal) {
	inputs.push_back(signal);
}

void LogicSimulator::Component::addOutput(std::uint32_t signal) {
	outputs.push_back(signal);
}

LogicSimulator::Circuit::Circuit(std::uint32_t inputSignalCount, std::uint32_t outputSignalCount)
	: inputSignalCount(inputSignalCount), outputSignalCount(outputSignalCount), signalCount(0) {
	//
}

std::uint32_t LogicSimulator::Circuit::allocateSignals(std::uint32_t count) {
	std::uint32_t baseSignal = signalCount;
	signalCount += count;
	return baseSignal;
}

void LogicSimulator::Circuit::addComponent(Component component) {
	components.emplace_back(std::move(component));
}

void LogicSimulator::outputCircuit(std::ofstream &f, const Circuit &circuit, const Options &options) {
    json j;

	int componentOffset = (circuit.signalCount - 1) * 2 + 2;
	int outputConnectionOffset = (circuit.signalCount - 1) * 2 + 2 + 7 + 2;

	int topOffset = 0;

	j["components"] = json::array();
	json &components = j["components"];

	for (const auto &component : circuit.components) {
		topOffset += 1;

		switch (component.type) {
			case ComponentType::Connect:
				break;
			case ComponentType::Const0:
			case ComponentType::Const1:
				components.push_back({
                    {"type", "const"},
                    {"x", componentOffset + 1},
                    {"y", topOffset},
                    {"value", component.type == ComponentType::Const1}
                });
				topOffset += 4;
				break;
			case ComponentType::AND:
				components.push_back({
					{"type", "and"},
					{"x", componentOffset + 1},
					{"y", topOffset},
					{"inputs", component.inputs.size()}
				});
				topOffset += 1 + (component.inputs.size() - 1) * 2 + 1;
				break;
			case ComponentType::OR:
				components.push_back({
					{"type", "or"},
					{"x", componentOffset + 1},
					{"y", topOffset},
					{"inputs", component.inputs.size()}
				});
				topOffset += 1 + (component.inputs.size() - 1) * 2 + 1;
				break;
			case ComponentType::XOR:
                components.push_back({
                    {"type", "xor"},
                    {"x", componentOffset + 1},
                    {"y", topOffset}
                });
                topOffset += 4;
                break;
			case ComponentType::NOT:
                components.push_back({
                    {"type", "not"},
                    {"x", componentOffset + 1},
                    {"y", topOffset}
                });
                topOffset += 4;
                break;
		}

		topOffset += 1;
	}

	if (options.ioComponents) {
	    for (std::uint32_t i = 0; i < circuit.inputSignalCount; i++) {
            components.push_back({
                {"type", "togglebutton"},
                {"x", -8},
                {"y", (int) i * 6 - (int) circuit.inputSignalCount * 6}
            });
	    }

        for (std::uint32_t i = 0; i < circuit.outputSignalCount; i++) {
            components.push_back({
                {"type", "led"},
                {"x", (circuit.inputSignalCount + circuit.outputSignalCount) * 2 + 1},
                {"y", (int) i * 6 - (int) circuit.outputSignalCount * 6}
            });
        }
	}

    j["connections"] = json::array();
    json &connections = j["connections"];

    topOffset = 0;

	for (const auto &component : circuit.components) {
	    bool skipConnections = false;

	    topOffset += 1;

	    switch (component.type) {
	        case ComponentType::Connect:
	            connections.push_back({
	                {"x1", component.inputs[0] * 2},
	                {"y1", topOffset},
	                {"x2", outputConnectionOffset + component.outputs[0] * 2},
	                {"y2", topOffset}
	            });
	            skipConnections = true;
	            break;
            case ComponentType::Const0:
            case ComponentType::Const1:
                topOffset += 4;
                break;
            case ComponentType::AND:
                topOffset += 1 + (component.inputs.size() - 1) * 2 + 1;
                break;
            case ComponentType::OR:
                topOffset += 1 + (component.inputs.size() - 1) * 2 + 1;
                break;
            case ComponentType::XOR:
                topOffset += 4;
                break;
            case ComponentType::NOT:
                topOffset += 4;
                break;
	    }

	    topOffset += 1;

	    if (skipConnections) {
            continue;
	    }

	    int maxPins = (int) std::max(component.inputs.size(), component.outputs.size());
	    int height = 1 + (maxPins - 1) * 2 + 1;
	    if (height < 4) {
	        height = 4;
	    }
	    int mid = height / 2;

	    int componentTopOffset = topOffset - 1 - height;
	    int inputTopOffset = componentTopOffset + 1 + mid - component.inputs.size();
	    int outputTopOffset = componentTopOffset + 1 + mid - component.outputs.size();

	    for (std::size_t i = 0; i < component.inputs.size(); i++) {
	        std::uint32_t inputSignal = component.inputs[i];

	        int y = inputTopOffset + i * 2;

	        connections.push_back({
	            {"x1", inputSignal * 2},
	            {"y1", y},
	            {"x2", componentOffset},
	            {"y2", y}
	        });
	    }

        for (std::size_t i = 0; i < component.outputs.size(); i++) {
            std::uint32_t outputSignal = component.outputs[i];

            int y = outputTopOffset + i * 2;

            connections.push_back({
                {"x1", outputConnectionOffset - 2},
                {"y1", y},
                {"x2", outputConnectionOffset + outputSignal * 2},
                {"y2", y}
            });
        }
	}

    for (std::uint32_t i = 0; i < circuit.signalCount; i++) {
        connections.push_back({
            {"x1", i * 2},
            {"y1", topOffset},
            {"x2", outputConnectionOffset + i * 2},
            {"y2", topOffset}
        });

        topOffset += 1;
    }

    for (std::uint32_t i = 0; i < circuit.signalCount; i++) {
        connections.push_back({
            {"x1", i * 2},
            {"y1", 0},
            {"x2", i * 2},
            {"y2", topOffset}
        });

        connections.push_back({
            {"x1", outputConnectionOffset + i * 2},
            {"y1", 0},
            {"x2", outputConnectionOffset + i * 2},
            {"y2", topOffset}
        });
    }

    if (options.ioComponents) {
        for (std::uint32_t i = 0; i < circuit.inputSignalCount; i++) {
            int x = i * 2;
            int y = (int) i * 6 - (int) circuit.inputSignalCount * 6 + 2;

            connections.push_back({
                {"x1", -2},
                {"y1", y},
                {"x2", x},
                {"y2", y}
            });

            connections.push_back({
                {"x1", x},
                {"y1", y},
                {"x2", x},
                {"y2", 0}
            });
        }

        for (std::uint32_t i = 0; i < circuit.outputSignalCount; i++) {
            int x = (circuit.inputSignalCount + i) * 2;
            int y = (int) i * 6 - (int) circuit.outputSignalCount * 6 + 2;

            connections.push_back({
                {"x1", x},
                {"y1", y},
                {"x2", (circuit.inputSignalCount + circuit.outputSignalCount) * 2},
                {"y2", y}
            });

            connections.push_back({
                {"x1", x},
                {"y1", y},
                {"x2", x},
                {"y2", 0}
            });
        }
    }

	f << j;
}

void LogicSimulator::generateBlock(const Intermediate::Block &block, Circuit &circuit, const Options &options) {
    std::uint32_t ioSignalCount = block.inputSignals + block.outputSignals;

    std::uint32_t publicBaseSignal = circuit.allocateSignals(ioSignalCount);

    std::vector<std::uint32_t> nestedBlocksBaseSignals;
    nestedBlocksBaseSignals.reserve(block.blocks.size());

    for (const auto &nestedBlock : block.blocks) {
    	nestedBlocksBaseSignals.push_back(circuit.signalCount);

    	generateBlock(*nestedBlock, circuit, options);
    }

    std::uint32_t localBaseSignal = circuit.signalCount;

    std::uint32_t localSignalCount = 0;

    // Inputs and outputs
	for (std::uint32_t i = 0; i < ioSignalCount; i++) {
		Component connector(ComponentType::Connect);
		connector.addInput(publicBaseSignal + i);
		connector.addOutput(localBaseSignal + i);
		circuit.addComponent(connector);

		localSignalCount++;
	}

	// Blocks
	for (std::uint32_t i = 0; i < block.blocks.size(); i++) {
		const auto &nestedBlock = block.blocks[i];
		std::uint32_t nestedBlockIoSignalCount = nestedBlock->inputSignals + nestedBlock->outputSignals;

		std::uint32_t nestedBlockBaseSignal = nestedBlocksBaseSignals[i];

		for (std::uint32_t j = 0; j < nestedBlockIoSignalCount; j++) {
			Component connector(ComponentType::Connect);
			connector.addInput(nestedBlockBaseSignal + j);
			connector.addOutput(localBaseSignal + localSignalCount);
			circuit.addComponent(connector);

			localSignalCount++;
		}
	}

	// Local signals
	for (const auto &statement : block.statements) {
		for (const auto &inputSignal : statement.inputs) {
			if (inputSignal >= localSignalCount) {
				localSignalCount = inputSignal + 1;
			}
		}

		for (const auto &outputSignal : statement.outputs) {
			if (outputSignal >= localSignalCount) {
				localSignalCount = outputSignal + 1;
			}
		}
	}

	circuit.allocateSignals(localSignalCount);

	for (const auto &statement : block.statements) {
		ComponentType componentType;

		switch (statement.op) {
			case Intermediate::Operation::Connect:
				componentType = ComponentType::Connect;
				break;
			case Intermediate::Operation::Const0:
				componentType = ComponentType::Const0;
				break;
			case Intermediate::Operation::Const1:
				componentType = ComponentType::Const1;
				break;
			case Intermediate::Operation::AND:
				componentType = ComponentType::AND;
				break;
			case Intermediate::Operation::OR:
				componentType = ComponentType::OR;
				break;
			case Intermediate::Operation::XOR:
				componentType = ComponentType::XOR;
				break;
			case Intermediate::Operation::NOT:
				componentType = ComponentType::NOT;
				break;
			default:
				throw CompilerError("Unsupported intermediate operation " + std::to_string(static_cast<int>(statement.op)));
		}

		Component component(componentType);

		for (const auto &inputSignal : statement.inputs) {
			component.addInput(localBaseSignal + inputSignal);
		}

		for (const auto &outputSignal : statement.outputs) {
			component.addOutput(localBaseSignal + outputSignal);
		}

		circuit.addComponent(component);
	}
}

void LogicSimulator::generateCircuit(std::ofstream &f, const Intermediate::File &intermediateFile, const Options &options) {
	Intermediate::Block *mainBlock = nullptr;

	for (const auto &block : intermediateFile.blocks) {
		if (block->name == "main") {
			mainBlock = block.get();
		}
	}

	if (mainBlock == nullptr) {
		throw CompilerError("'main' block not defined");
	}

	Circuit circuit(mainBlock->inputSignals, mainBlock->outputSignals);

	generateBlock(*mainBlock, circuit, options);

	outputCircuit(f, circuit, options);
}

void LogicSimulator::run(std::string outputFilename, const HDLCompiler::Intermediate::File &intermediateFile, const std::vector<std::string> &argv) {
	Options options {
		.ioComponents = true
	};

	for (const auto &arg : argv) {
		if (arg == "--no-io-components") {
			options.ioComponents = false;
		} else {
			throw CompilerError("Unrecognized backend option '" + arg + "'");
		}
	}

	if (outputFilename.empty()) {
		outputFilename = "circuit.json";
	}

	std::ofstream outputStream(outputFilename);
	if (!outputStream) {
		throw CompilerError("Failed to open " + outputFilename + ": " + std::strerror(errno));
	}

	generateCircuit(outputStream, intermediateFile, options);
}

}

}

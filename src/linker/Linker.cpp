
#include "Linker.h"

#include <shared/Intermediate.h>
#include <shared/errors.h>

namespace HDLCompiler {

void Linker::linkTwo(Intermediate::File &target, const Intermediate::File &source) {
	target.blocks.reserve(target.blocks.size() + source.blocks.size());

	for (const auto &sourceBlock : source.blocks) {
		for (const auto &targetBlock : target.blocks) {
			if (sourceBlock->name == targetBlock->name) {
				throw CompilerError("Linker: duplicate definition of block \"" + sourceBlock->name + "\"");
			}
		}

		target.blocks.push_back(sourceBlock);
	}
}

Intermediate::File Linker::link(const std::vector<Intermediate::File> &inputs) {
	if (inputs.empty()) {
		return Intermediate::File();
	} else {
		Intermediate::File output(inputs[0]);

		for (std::size_t i = 1; i < inputs.size(); i++) {
			linkTwo(output, inputs[i]);
		}

		return output;
	}
}

}

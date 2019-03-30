
#pragma once

#include <shared/Intermediate.h>

namespace HDLCompiler {

namespace Intermediate {

class StringTable;

struct File {
	static const std::uint32_t VERSION = 3;

	std::vector<std::shared_ptr<Block>> blocks;

	File() = default;
	explicit File(std::vector<std::shared_ptr<Block>> blocks);

	static File read(const std::string &filename);

	void write(const std::string &filename) const;

    void print(std::ostream &f) const;

private:
	static File readFile(std::ifstream &f);
	static std::vector<std::shared_ptr<Block>> readBlocks(std::ifstream &f, const StringTable &stringTable);
	static Block readBlock(std::ifstream &f, const StringTable &stringTable);
	static Statement readBlockStatement(std::ifstream &f);

	void writeFile(std::ofstream &f) const;
    void writeBlocks(std::ofstream &f, StringTable &stringTable) const;
    void writeBlock(std::ofstream &f, const Block &block, StringTable &stringTable) const;
	void writeBlockStatement(std::ofstream &f, const Statement &statement) const;
};

}

}

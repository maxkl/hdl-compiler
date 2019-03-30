
#include "IntermediateFile.h"

#include <fstream>
#include <cstring>

#include <shared/Intermediate.h>
#include <shared/helper/BufferHelper.h>
#include <shared/errors.h>

namespace HDLCompiler {

namespace Intermediate {

static const std::uint8_t magic[] = {
//      'H',  'D',  'L',  'I'
        0x48, 0x44, 0x4c, 0x49
};

void readBuffer(std::ifstream &f, std::uint8_t *buffer, std::size_t size) {
    f.read(reinterpret_cast<char *>(buffer), size);
    std::streamsize bytesRead = f.gcount();
    if (bytesRead != size) {
        if (f.eof()) {
            throw CompilerError("Reached EOF while reading from file");
        }
        throw CompilerError("Error while reading from file");
    }
}

std::uint8_t read8(std::ifstream &f) {
    std::uint8_t buffer[1];
    readBuffer(f, buffer, sizeof(buffer));
    return BufferHelper::read8(buffer, 0);
}

std::uint16_t read16le(std::ifstream &f) {
    std::uint8_t buffer[2];
    readBuffer(f, buffer, sizeof(buffer));
    return BufferHelper::read16le(buffer, 0);
}

std::uint32_t read32le(std::ifstream &f) {
    std::uint8_t buffer[4];
    readBuffer(f, buffer, sizeof(buffer));
    return BufferHelper::read32le(buffer, 0);
}

std::uint64_t read64le(std::ifstream &f) {
    std::uint8_t buffer[8];
    readBuffer(f, buffer, sizeof(buffer));
    return BufferHelper::read64le(buffer, 0);
}

std::int64_t read64sle(std::ifstream &f) {
    return static_cast<std::int64_t>(read64le(f));
}

void writeBuffer(std::ofstream &f, const std::uint8_t *buffer, std::size_t size) {
    f.write(reinterpret_cast<const char *>(buffer), size);
    if (!f) {
        throw CompilerError("Error while writing to file");
    }
}

void write8le(std::ofstream &f, std::uint8_t value) {
    std::uint8_t buffer[1];
    BufferHelper::write8(buffer, 0, value);
    writeBuffer(f, buffer, sizeof(buffer));
}

void write16le(std::ofstream &f, std::uint16_t value) {
    std::uint8_t buffer[2];
    BufferHelper::write16le(buffer, 0, value);
    writeBuffer(f, buffer, sizeof(buffer));
}

void write32le(std::ofstream &f, std::uint32_t value) {
    std::uint8_t buffer[4];
    BufferHelper::write32le(buffer, 0, value);
    writeBuffer(f, buffer, sizeof(buffer));
}

void write64le(std::ofstream &f, std::uint64_t value) {
    std::uint8_t buffer[8];
    BufferHelper::write64le(buffer, 0, value);
    writeBuffer(f, buffer, sizeof(buffer));
}

void write64sle(std::ofstream &f, std::int64_t value) {
    write64le(f, static_cast<std::uint64_t>(value));
}

struct StringTable {
    std::vector<std::string> strings;
    std::uint32_t totalSize;

    StringTable()
        : totalSize(0) {
        //
    }

    explicit StringTable(std::vector<std::string> strings)
        : strings(std::move(strings)), totalSize(calculateTotalSize(this->strings)) {
        //
    }

    std::uint32_t add(const std::string &str) {
        std::size_t strSizeWithNull = str.size() + 1;
        if (totalSize > UINT32_MAX - strSizeWithNull) {
            throw CompilerError("String table has reached maximum size");
        }
        unsigned long index = strings.size();
        if (index >= UINT32_MAX) {
            throw CompilerError("String table has reached maximum number of strings");
        }
        strings.push_back(str);
        totalSize += strSizeWithNull;
        return static_cast<std::uint32_t>(index);
    }

    void write(std::ofstream &f) {
        write32le(f, totalSize);

        for (const auto &string : strings) {
            writeBuffer(f, reinterpret_cast<const std::uint8_t *>(string.c_str()), string.size() + 1);
        }
    }

    static StringTable read(std::ifstream &f) {
        std::vector<std::string> strings;

        std::uint32_t totalSize = read32le(f);

        std::unique_ptr<std::uint8_t[]> buffer(new std::uint8_t[totalSize]);
        readBuffer(f, buffer.get(), totalSize);

        std::uint32_t startIndex = 0;
        for (std::uint32_t i = 0; i < totalSize; i++) {
            if (buffer[i] == '\0') {
                strings.emplace_back(reinterpret_cast<char *>(&buffer[startIndex]), i - startIndex);
                startIndex = i + 1;
            }
        }

        if (startIndex != totalSize) {
            throw CompilerError("Last string in string table not null terminated");
        }

        return StringTable(strings);
    }

private:
    static std::uint32_t calculateTotalSize(const std::vector<std::string> &strings) {
        std::size_t totalSize = 0;
        for (const auto &string : strings) {
            totalSize += string.size() + 1;
        }
        if (totalSize > UINT32_MAX) {
            throw CompilerError("Strings exceed maximum size");
        }
        return static_cast<std::uint32_t>(totalSize);
    }
};

std::uint16_t mapIntermediateToFileOp(Operation op) {
    // TODO: properly map to/from file format specific op code
    return static_cast<std::uint16_t>(op);
}

Operation mapFileToIntermediateOp(std::uint16_t op) {
    // TODO: properly map to/from file format specific op code
    return static_cast<Operation>(op);
}

Statement File::readBlockStatement(std::ifstream &f) {
    std::uint16_t op = read16le(f);

    std::uint16_t size = read16le(f);

    Statement statement(mapFileToIntermediateOp(op), size);

    for (std::uint32_t i = 0; i < statement.inputs.size(); i++) {
        std::uint32_t inputSignal = read32le(f);
        statement.setInput(i, inputSignal);
    }

    for (std::uint32_t i = 0; i < statement.outputs.size(); i++) {
        std::uint32_t outputSignal = read32le(f);
        statement.setOutput(i, outputSignal);
    }

    return statement;
}

void File::writeBlockStatement(std::ofstream &f, const Statement &statement) const {
    write16le(f, mapIntermediateToFileOp(statement.op));

    write16le(f, statement.size);

    for (const auto &inputSignal : statement.inputs) {
        write32le(f, inputSignal);
    }

    for (const auto &outputSignal : statement.outputs) {
        write32le(f, outputSignal);
    }
}

Block File::readBlock(std::ifstream &f, const StringTable &stringTable) {
    std::uint32_t nameIndex = read32le(f);
    std::string name = stringTable.strings[nameIndex];

    std::uint32_t inputSignals = read32le(f);

    std::uint32_t outputSignals = read32le(f);

    std::uint32_t blockCount = read32le(f);

    std::uint32_t statementCount = read32le(f);

    // TODO: blocks
    std::vector<std::shared_ptr<Block>> blocks;

    std::vector<Statement> statements;
    statements.reserve(statementCount);

    for (std::uint32_t i = 0; i < statementCount; i++) {
        statements.push_back(readBlockStatement(f));
    }

    return Block(name, inputSignals, outputSignals, blocks, statements);
}

void File::writeBlock(std::ofstream &f, const Block &block, StringTable &stringTable) const {
    write32le(f, stringTable.add(block.name));

    write32le(f, block.inputSignals);

    write32le(f, block.outputSignals);

    std::size_t blockCount = block.blocks.size();
    if (blockCount > UINT32_MAX) {
        throw CompilerError("Block references too many blocks");
    }
    write32le(f, static_cast<std::uint32_t>(blockCount));

    std::size_t statementCount = block.statements.size();
    if (blockCount > UINT32_MAX) {
        throw CompilerError("Block has too many statements");
    }
    write32le(f, static_cast<std::uint32_t>(statementCount));

    // TODO: blocks

    for (const auto &statement : block.statements) {
        writeBlockStatement(f, statement);
    }
}

std::vector<std::shared_ptr<Block>> File::readBlocks(std::ifstream &f, const StringTable &stringTable) {
    std::uint32_t blockCount = read32le(f);

    std::vector<std::shared_ptr<Block>> blocks;
    blocks.reserve(blockCount);

    for (std::uint32_t i = 0; i < blockCount; i++) {
        blocks.push_back(std::make_shared<Block>(std::move(readBlock(f, stringTable))));
    }

    return blocks;
}

void File::writeBlocks(std::ofstream &f, StringTable &stringTable) const {
    std::size_t blockCount = blocks.size();
    if (blockCount > UINT32_MAX) {
        throw CompilerError("Too many blocks");
    }
    write32le(f, static_cast<std::uint32_t>(blockCount));

    for (const auto &block : blocks) {
        writeBlock(f, *block, stringTable);
    }
}

File File::readFile(std::ifstream &f) {
    // Compare file magic numbers
    std::uint8_t fileMagic[sizeof(magic)];
    readBuffer(f, fileMagic, sizeof(magic));
    for (std::size_t i = 0; i < sizeof(magic); i++) {
        if (fileMagic[i] != magic[i]) {
            throw CompilerError("File is not an intermediate file (magic mismatch)");
        }
    }

    // Currently we can only read files with the exact same version
    uint32_t version = read32le(f);
    if (version != VERSION) {
        throw CompilerError("File has incompatible format (version mismatch)");
    }

    // Get location of string table
    std::int64_t stringTablePointer = read64sle(f);

    // End of fixed-size header

    // We read the string table before anything else so that we can immediately resolve strings as we encounter them

    // Jump to string table
    f.seekg(stringTablePointer);

    StringTable stringTable = StringTable::read(f);

    std::vector<std::shared_ptr<Block>> blocks = readBlocks(f, stringTable);

    return File(blocks);
}

void File::writeFile(std::ofstream &f) const {
    // Write file magic
    writeBuffer(f, magic, sizeof(magic));

    // Write file format version
    write32le(f, VERSION);

    // Reserve space for pointer to string table
    auto stringTablePointerPosition = f.tellp();
    write64le(f, 0);

    // End of fixed-size header

    StringTable stringTable;

    writeBlocks(f, stringTable);

    // Store current file pointer because this is the start of the string table
    std::int64_t stringTablePointer = f.tellp();

    stringTable.write(f);

    // Jump back to beginning to write location of string table to header
    f.seekp(stringTablePointerPosition);
    write64sle(f, stringTablePointer);
}

File File::read(const std::string &filename) {
    std::ifstream f(filename, std::ios::binary);
    if (!f) {
        throw CompilerError("Failed to open " + filename + ": " + std::strerror(errno));
    }

    return readFile(f);
}

void File::write(const std::string &filename) const {
    std::ofstream f(filename, std::ios::binary);
    if (!f) {
        throw CompilerError("Failed to open " + filename + ": " + std::strerror(errno));
    }

    writeFile(f);
}

File::File(std::vector<std::shared_ptr<Block>> blocks)
    : blocks(std::move(blocks)) {
    //
}

void File::print(std::ostream &f) const {
    for (const auto &block : blocks) {
        block->print(f);
    }
}

}

}

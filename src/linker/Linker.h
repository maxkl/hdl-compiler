
#pragma once

#include <shared/IntermediateFile.h>

namespace HDLCompiler {

class Linker {
public:
    static Intermediate::File link(const std::vector<Intermediate::File> &inputs);

private:
    static void linkTwo(Intermediate::File &target, const Intermediate::File &source);
};

}

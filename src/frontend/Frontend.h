
#pragma once

#include <shared/IntermediateFile.h>

namespace HDLCompiler {

class Frontend {
public:
    static Intermediate::File compile(const std::string &filename);
};

}


#pragma once

#include <shared/helper/StackTraceError.h>

namespace HDLCompiler {

class CompilerError : public StackTraceError {
public:
    explicit CompilerError(const std::string &msg)
        : StackTraceError(msg) {
        //
    }
};

}
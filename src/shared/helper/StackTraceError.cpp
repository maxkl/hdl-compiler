
#include "StackTraceError.h"

#include <memory>

#include "stacktrace.h"

StackTraceError::StackTraceError(const std::string &msg) {
    stacktrace = backtrace();

    message = msg;
    for (const auto &line : stacktrace) {
        message += "\n" + line;
    }
}

const char *StackTraceError::what() const noexcept {
    return message.c_str();
}

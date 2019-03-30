
#include "StackTraceError.h"

#include <memory>

#include "stacktrace.h"

StackTraceError::StackTraceError(const std::string &msg) {
    stacktrace_ = backtrace();

    message = msg;

    // Only append stack trace to message for debug builds
#ifndef NDEBUG
    for (const auto &line : stacktrace_) {
        message += "\n" + line;
    }
#endif
}

const std::vector<std::string> StackTraceError::stacktrace() {
    return stacktrace_;
}

const char *StackTraceError::what() const noexcept {
    return message.c_str();
}


#pragma once

#include <stdexcept>
#include <vector>
#include <string>

/**
 * Generic exception class that captures a stack trace. The stack trace is included in the message only for debug builds
 */
class StackTraceError : public std::exception {
public:
    explicit StackTraceError(const std::string &msg);

    const char *what() const noexcept override;

    const std::vector<std::string> stacktrace();

protected:
    std::vector<std::string> stacktrace_;

private:
    std::string message;
};

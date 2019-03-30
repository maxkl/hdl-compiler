
#pragma once

#include <stdexcept>
#include <vector>
#include <string>

class StackTraceError : public std::exception {
public:
    explicit StackTraceError(const std::string &msg);

    const char *what() const noexcept override;

protected:
    std::vector<std::string> stacktrace;

private:
    std::string message;
};

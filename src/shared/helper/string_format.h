
#pragma once

#include <string>
#include <memory>

/**
 * printf-like formatting for C++ with std::string
 * Source: https://stackoverflow.com/a/26221725
 */
template<typename ... Args>
__attribute__((format(printf, 1, 0)))
std::string string_format(const char *format, Args ... args) {
    // Determine required buffer size (extra space for '\0')
    std::size_t size = std::snprintf(nullptr, 0, format, args ...) + 1;
    // Allocate char buffer
    std::unique_ptr<char[]> buf(new char[size]);
    // Format string and store it into correctly sized buffer
    std::snprintf(buf.get(), size, format, args ...);
    // Construct string from char buffer (without the '\0')
    return std::string(buf.get(), buf.get() + size - 1);
}

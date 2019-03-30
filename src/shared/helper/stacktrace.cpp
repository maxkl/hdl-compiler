
#include "stacktrace.h"

#define UNW_LOCAL_ONLY

#include <cstdlib>
#include <cstdio>
#include <cxxabi.h>
#include <libunwind.h>

#include "string_format.h"

/**
 *  Adapted from https://eli.thegreenplace.net/2015/programmatic-access-to-the-call-stack-in-c/
 */
std::vector<std::string> backtrace() {
    std::vector<std::string> stacktrace;

    unw_context_t context;
    if (unw_getcontext(&context) != 0) {
        stacktrace.emplace_back(" -- error: unable to obtain machine-state for stack trace");
        return stacktrace;
    }

    unw_cursor_t cursor;
    if (unw_init_local(&cursor, &context) != 0) {
        stacktrace.emplace_back(" -- error: unable to initialize cursor for stack trace");
        return stacktrace;
    }

    int skip = 0;
    while (unw_step(&cursor) > 0) {
        if (skip > 0) {
            skip--;
            continue;
        }

        unw_word_t offset, pc;
        if (unw_get_reg(&cursor, UNW_REG_IP, &pc) == 0) {
            std::string line = string_format("0x%lx", pc);

            char sym[256];
            int ret = unw_get_proc_name(&cursor, sym, sizeof(sym), &offset);
            if (ret == 0) {
                char *nameptr = sym;
                int status;
                char *demangled = abi::__cxa_demangle(sym, nullptr, nullptr, &status);
                if (status == 0) {
                    nameptr = demangled;
                }
                line += string_format(" in %s (+0x%lx)", nameptr, offset);
                std::free(demangled);
            } else if (ret == -UNW_ENOMEM) {
                line += string_format(" in %s... (+0x%lx)", sym, offset);
            } else {
                line += " -- error: unable to obtain symbol name for this frame";
            }
            stacktrace.push_back(line);
        } else {
            stacktrace.emplace_back(" -- error: unable to obtain program counter for this frame");
        }
    }

    return stacktrace;
}
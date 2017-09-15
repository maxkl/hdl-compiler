
#pragma once

#include <stdio.h>

#define log_info(...) printf(__VA_ARGS__)

#define log_error(...) fprintf(stderr, __VA_ARGS__)

#ifdef NDEBUG
#define log_debug(...)
#else
#define log_debug(...) printf(__VA_ARGS__)
#endif

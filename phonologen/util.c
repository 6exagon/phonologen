#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "util.h"

inline void fail_if(char condition, const char *fmt, ...) {
    if (condition) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        exit(EXIT_FAILURE);
    }
}

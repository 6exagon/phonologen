#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>

// If char condition is false, prints the remaining args to stderr (one format
// string followed by its variable arguments) and exits with EXIT_FAILURE
void fail_if(char, const char *, ...);

#endif

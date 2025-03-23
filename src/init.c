#include <stdio.h>
#include <stdarg.h>

#include "init.h"

// Debug print function for cleaner debug output handling.
// Prints only if DEBUG is enabled.
void debugPrint(const char *format, ...) {
    if (DEBUG) {
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
    }
}
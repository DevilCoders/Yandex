#include "vbox_common.h"

#include <stdarg.h>
#include <stdio.h>

int logme(ELogType /*log*/, const char *fmt, ...) {
    return 0;
    va_list ap;
    va_start(ap, fmt);
    int n = vfprintf(stderr, fmt, ap);
    n += fprintf(stderr, "\n");
    va_end(ap);
    return n;
}

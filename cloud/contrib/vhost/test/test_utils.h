#include <stdarg.h>
#include "vhost/server.h"

/* Normally we pass LOG_VERBOSITY from make */
#ifndef LOG_VERBOSITY
#   define LOG_VERBOSITY LOG_INFO
#endif

/* Log function for tests */
static const char *log_level_str[] = {
    "ERROR",
    "WARNING",
    "INFO",
    "DEBUG"
};

static inline void vhd_log_stderr(enum LogLevel level, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    if (level <= LOG_VERBOSITY) {
        fprintf(stderr, "%s: ", log_level_str[level]);
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
    }
    va_end(args);
}

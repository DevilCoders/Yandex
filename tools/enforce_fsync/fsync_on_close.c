/*
 * LD_PRELOAD-style library which forces fsync() before close() for
 * not-temporary files.
 *
 * How it works.
 *
 * At first, place hooks for close()/fclose() and functions which create a
 * new file descriptor (*open/dup/etc).  When a new file descriptor
 * creation is intercepted, save descriptors for non-temporary files (every
 * filename which doesn't contain "tmp"/"temp" is non-temporary).  When
 * close() is intercepted, call fsync() for non-temporary file descriptors
 * before calling close().
 *
 * Usage:
 *     export LD_PRELOAD=/path/to/libfsynconclose.so
 *     prog_without_fsync
 */

#if defined(__FreeBSD__) || defined(_darwin_)

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>

/* Ad-hoc implementation of bit vector */
#define T uint64_t
#define ELEMSZ (sizeof(T)*8)
#define NELEM 128
static T fds[NELEM];

#if TRACE_FSYNC
static char fns[NELEM*ELEMSZ][PATH_MAX];
#endif

static void
MarkFD(const char *filename, int fd)
{
    (void)filename;
    if ((T)fd < NELEM * ELEMSZ) {
        fds[fd / ELEMSZ] |= ((T)1 << (fd % ELEMSZ));
#if TRACE_FSYNC
        strncpy(fns[fd], filename, PATH_MAX)[PATH_MAX-1] = '\0';
        fprintf(stderr, "Remembered %s\n", fns[fd]);
#endif
    }
}

#define bool T
#define false 0
#define true 1

static bool
CheckAndResetMark(int fd)
{
    bool ret = true;

    if ((T)fd < NELEM * ELEMSZ) {
#if TRACE_FSYNC
        fprintf(stderr, "Flushing %s\n", fns[fd]);
        fns[fd][0] = '\0';
#endif
        ret = fds[fd / ELEMSZ] & ((T)1 << (fd % ELEMSZ));
        if (ret)
            fds[fd / ELEMSZ] &= ~((T)1 << (fd % ELEMSZ));
    }
    return ret;
}

int
IsTmpFile(const char *fn)
{
    return fn && (strstr(fn, "tmp") || strstr(fn, "temp") || !strncmp(fn, "/dev/", 5) || !strncmp(fn, "/proc/", 6));
}

static int (*RealOpen)(const char *, int, int);
static int (*RealOpenAt)(int, const char *, int, int);
static int (*RealCreat)(const char *, int);
static FILE* (*RealFopen)(const char *, const char*);
static FILE* (*RealFreopen)(const char *, const char*, FILE*);
static int (*RealFclose)(FILE *);

int (*RealClose)(int);

#define SAVE_ERRNO do { int saved_errno = errno;
#define RESTORE_ERRNO errno = saved_errno; } while(0);

int creat(const char *filename, mode_t mode)
{
    int result;

    if (!RealCreat)
        RealCreat = dlsym(RTLD_NEXT, "creat");

    result = RealCreat(filename, mode);
    SAVE_ERRNO
    if (result >= 0 && ! IsTmpFile(filename))
        MarkFD(filename, result);
    RESTORE_ERRNO
    return result;
}

int open(const char *filename, int flags, ...)
{
    int result;
    va_list ap;
    int mode = 0;

    va_start(ap, flags);
    if (flags & O_CREAT) {
        mode = va_arg(ap, int);
    }
    va_end(ap);

    if (!RealOpen)
        RealOpen = dlsym(RTLD_NEXT, "open");

    result = RealOpen(filename, flags, mode);
    SAVE_ERRNO
    if (result >= 0 && ! IsTmpFile(filename))
        MarkFD(filename, result);
    RESTORE_ERRNO
    return result;
}

int openat(int dirfd, const char *filename, int flags, ...)
{
    int result;
    va_list ap;
    int mode = 0;

    va_start(ap, flags);
    if (flags & O_CREAT) {
        mode = va_arg(ap, int);
    }
    va_end(ap);

    if (!RealOpenAt)
        RealOpenAt = dlsym(RTLD_NEXT, "openat");

    result = RealOpenAt(dirfd, filename, flags, mode);
    SAVE_ERRNO
    if (result >= 0 && ! IsTmpFile(filename))
        MarkFD(filename, result);
    RESTORE_ERRNO
    return result;
}

int close(int fd)
{
    int result;

    if (!RealClose)
        RealClose = dlsym(RTLD_NEXT, "close");

    if (CheckAndResetMark(fd)) {
        result = fsync(fd);
        if (result < 0)
            return result;
    }
    result = RealClose(fd);
    return result;
}

FILE* fopen(const char* filename, const char* mode)
{
    FILE* result;

    if (!RealFopen)
        RealFopen = dlsym(RTLD_NEXT, "fopen");

    result = RealFopen(filename, mode);
    SAVE_ERRNO
    if (result != NULL && ! IsTmpFile(filename))
        MarkFD(filename, fileno(result));
    RESTORE_ERRNO
    return result;
}

FILE* freopen(const char* filename, const char* mode, FILE *stream)
{
    FILE* result;
    int ret;
    bool wasMarked = false;
    int fd = fileno(stream);

    if (!RealFreopen)
        RealFreopen = dlsym(RTLD_NEXT, "freopen");

    if (CheckAndResetMark(fd)) {
        ret = fflush(stream);
        if (ret != 0 && errno != EBADF)
            return NULL;
        ret = fsync(fd);
        if (ret < 0)
            return NULL;
        wasMarked = true;
    }
    result = RealFreopen(filename, mode, stream);
    SAVE_ERRNO
    if (result != NULL && (wasMarked || ! IsTmpFile(filename)))
        MarkFD(filename, fileno(result));
    RESTORE_ERRNO
    return result;
}

int fclose(FILE *stream)
{
    int result;
    int fd = fileno(stream);

    if (!RealFclose)
        RealFclose = dlsym(RTLD_NEXT, "fclose");

    if (CheckAndResetMark(fd)) {
        result = fflush(stream);
        if (result != 0 && errno != EBADF)
            return result;
        result = fsync(fd);
        if (result < 0)
            return EOF;
    }
    result = RealFclose(stream);
    return result;
}

#endif

/* vim:set ts=4 sw=4 et tw=75: */

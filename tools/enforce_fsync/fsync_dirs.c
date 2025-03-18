/*
 * LD_PRELOAD-style library which forces fsync() after directory changing
 * operations.
 *
 * How it works.
 *
 * !!!!!!!!!!!!!!!! TODO !!!!!!!!!
 *
 * Usage:
 *     export LD_PRELOAD=/path/to/libfsynconclose.so
 *     prog_without_fsync
 */

#if defined(__FreeBSD__) || defined(_darwin_)

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>

static int (*RealRename)(const char *, const char *);
// static int (*RealRenameAt)(int, const char *, int, const char *);
static int (*RealLink)(const char *, const char *);
// static int (*RealLinkAt)(int, const char *, int, const char *, int);
static int (*RealSymlink)(const char *, const char *);
// static int (*RealSymlinkAt)(const char *, int, const char *);
static int (*RealUnlink)(const char *);
// static int (*RealUnlinkAt)(int, const char *, int);
static int (*RealMkdir)(const char *, mode_t);
// static int (*RealMkdirAt)(int, const char *, mode_t);
static int (*RealRmdir)(const char *);
// static int (*RealMknod)(const char *path, mode_t mode, dev_t dev);

extern int (*RealClose)(int);

#define SAVE_ERRNO do { int saved_errno = errno;
#define RESTORE_ERRNO errno = saved_errno; } while(0);

extern int
IsTmpFile(const char *fn);

static int
OpenParentDir(const char *path)
{
    int ret = -1;
    const char *parent;

    // TODO: resolve symlinks?
    parent = dirname(path);
    if (parent) {
#ifdef TRACE_FSYNC
        SAVE_ERRNO
        fprintf(stderr, "fsync('%s')\n", parent);
        RESTORE_ERRNO
#endif
        ret = open(parent, 0);
    }

    return ret;
}

static int
FsyncParentDir(const char *path)
{
    int fd;
    int result;

    if (!IsTmpFile(path)) {
        if (!RealClose)
            RealClose = dlsym(RTLD_NEXT, "close");

        fd = OpenParentDir(path);
        if (fd < 0)
            return fd;
        result = fsync(fd);
        if (result < 0) {
            SAVE_ERRNO
            RealClose(fd);
            RESTORE_ERRNO
            return result;
        } else {
            return RealClose(fd);
        }
    }
    return 0;
}

static int
FsyncParentDir2(const char *path1, const char *path2)
{
    int result;

    // TODO: single fsync for the same parent dir of both path1 & path2
    result = FsyncParentDir(path1);
    if (result >= 0)
        result = FsyncParentDir(path2);

    return result;
}

int
unlink(const char *path)
{
    int result;

    if (!RealUnlink)
        RealUnlink = dlsym(RTLD_NEXT, "unlink");

    result = RealUnlink(path);
    if (result >= 0)
        result = FsyncParentDir(path);
    return result;
}

int
rmdir(const char *path)
{
    int result;

    if (!RealRmdir)
        RealRmdir = dlsym(RTLD_NEXT, "rmdir");

    result = RealRmdir(path);
    if (result >= 0)
        result = FsyncParentDir(path);
    return result;
}

int
symlink(const char *path1, const char *path2)
{
    int result;

    if (!RealSymlink)
        RealSymlink = dlsym(RTLD_NEXT, "symlink");

    result = RealSymlink(path1, path2);
    if (result >= 0)
        result = FsyncParentDir(path2);
    return result;
}

int
link(const char *path1, const char *path2)
{
    int result;

    if (!RealLink)
        RealLink = dlsym(RTLD_NEXT, "link");

    result = RealLink(path1, path2);
    if (result >= 0)
        result = FsyncParentDir(path2);
    return result;
}

int
rename(const char *old, const char *new)
{
    int result;

    if (!RealRename)
        RealRename = dlsym(RTLD_NEXT, "rename");

    result = RealRename(old, new);
    if (result >= 0)
        result = FsyncParentDir2(old, new);
    return result;
}

int
mkdir(const char *path, mode_t mode)
{
    int result;

    if (!RealMkdir)
        RealMkdir = dlsym(RTLD_NEXT, "mkdir");

    result = RealMkdir(path, mode);
    if (result >= 0)
        result = FsyncParentDir(path);
    return result;
}

#endif

/* vim:set ts=4 sw=4 et tw=75: */

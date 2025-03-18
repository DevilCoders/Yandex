#include "vbox_fs_wrapper.h"

#include "vbox_common.h"

#include <linux/limits.h>

#include <util/system/env.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *vboxpath(const char *path, char *buf, int /*bufsz*/) {
    const TString root = GetEnv("VBOX_ROOT");
    if (*path == '/' && strncmp(path, root.data(), strlen(root.data())) != 0) {
        sprintf(buf, "%s/%s/%s", root.data(), GetEnv("HOSTNAME").data(), path);
        logme(fs, "vboxpath = %s", buf);
        return buf;
    }
    strcpy(buf, path);
    logme(fs, "vboxpath = %s", buf);
    return buf;
}

DIR *opendir_wrapper(const char *name) {
    logme(fs, "opendir(%s)", name);

    typedef DIR *libc_opendir_fn(const char *name);
    static libc_opendir_fn *libc_opendir = nullptr;
    if (!libc_opendir) libc_opendir = (libc_opendir_fn*) dlsym(RTLD_NEXT, "opendir");

    char vpath[PATH_MAX];
    vboxpath(name, vpath, PATH_MAX);

    DIR *rc = libc_opendir(vpath);
    if (rc) return rc;

    return libc_opendir(name);
}

int open_wrapper(const char *pathname, int flags, ...) {
    logme(fs, "open(%s, %d)", pathname, flags);

    mode_t mode = 0;
    if (flags | O_CREAT) {
        va_list ap;
        va_start(ap,flags);
        mode = va_arg(ap, mode_t);
        logme(fs, "open() mode %d", mode);
        va_end(ap);
    }

    typedef int libc_open_fn(const char *pathname, int flags, ...);
    static libc_open_fn *libc_open = nullptr;
    if (!libc_open) libc_open = (libc_open_fn*) dlsym(RTLD_NEXT, "open");

    char vpath[PATH_MAX];
    vboxpath(pathname, vpath, PATH_MAX);

    int rc = libc_open(vpath, flags, mode);
    if (rc >= 0) return rc;

    return libc_open(pathname, flags, mode);
}

int open64_wrapper(const char *pathname, int flags, ...) {
    logme(fs, "open64(%s, %d)", pathname, flags);

    mode_t mode = 0;
    if (flags | O_CREAT) {
        va_list ap;
        va_start(ap,flags);
        mode = va_arg(ap, mode_t);
        logme(fs, "open64() mode %d", mode);
        va_end(ap);
    }

    typedef int libc_open64_fn(const char *pathname, int flags, ...);
    static libc_open64_fn *libc_open64 = nullptr;
    if (!libc_open64) libc_open64 = (libc_open64_fn*) dlsym(RTLD_NEXT, "open64");

    char vpath[PATH_MAX];
    vboxpath(pathname, vpath, PATH_MAX);

    int rc = libc_open64(vpath, flags, mode);
    if (rc >= 0) return rc;

    return libc_open64(pathname, flags, mode);
}

FILE *fopen_wrapper(const char *path, const char *mode) {
    logme(fs, "fopen(%s, %s)", path, mode);

    typedef FILE* libc_fopen_fn(const char *path, const char *mode);
    static libc_fopen_fn *libc_fopen = nullptr;
    if (!libc_fopen) libc_fopen = (libc_fopen_fn*) dlsym(RTLD_NEXT, "fopen");

    char vpath[PATH_MAX];
    vboxpath(path, vpath, PATH_MAX);

    FILE *rc = libc_fopen(vpath, mode);
    if (rc) return rc;

    return libc_fopen(path, mode);
}

FILE *fopen64_wrapper(const char *path, const char *mode) {
    logme(fs, "fopen64(%s, %s)", path, mode);

    typedef FILE* libc_fopen64_fn(const char *pathname, const char *mode);
    static libc_fopen64_fn *libc_fopen64 = nullptr;
    if (!libc_fopen64) libc_fopen64 = (libc_fopen64_fn*) dlsym(RTLD_NEXT, "fopen64");

    char vpath[PATH_MAX];
    vboxpath(path, vpath, PATH_MAX);

    FILE *rc = libc_fopen64(vpath, mode);
    if (rc) return rc;

    return libc_fopen64(path, mode);
}

int link_wrapper(const char *oldpath, const char *newpath) {
    logme(fs, "link(%s, %s)", oldpath, newpath);

    typedef int libc_link_fn(const char *oldpath, const char *newpath);
    static libc_link_fn *libc_link = nullptr;
    if (!libc_link) libc_link = (libc_link_fn*) dlsym(RTLD_NEXT, "link");

    char vpath1[PATH_MAX];
    vboxpath(oldpath, vpath1, PATH_MAX);

    char vpath2[PATH_MAX];
    vboxpath(newpath, vpath2, PATH_MAX);

    int rc = libc_link(vpath1, vpath2);
    if (rc == 0) return rc;

    return libc_link(oldpath, newpath);
}


int symlink_wrapper(const char *oldpath, const char *newpath) {
    logme(fs, "symlink(%s, %s)", oldpath, newpath);

    typedef int libc_symlink_fn(const char *oldpath, const char *newpath);
    static libc_symlink_fn *libc_symlink = nullptr;
    if (!libc_symlink) libc_symlink = (libc_symlink_fn*) dlsym(RTLD_NEXT, "symlink");

    char vpath1[PATH_MAX];
    vboxpath(oldpath, vpath1, PATH_MAX);

    char vpath2[PATH_MAX];
    vboxpath(newpath, vpath2, PATH_MAX);

    int rc = libc_symlink(vpath1, vpath2);
    if (rc == 0) return rc;

    return libc_symlink(oldpath, newpath);
}


int __xstat_wrapper(int vers, const char *path, struct stat *buf) {
    logme(fs, "__xstat(%d, %s)", vers, path);

    typedef int libc___xstat_fn(int vers, const char *path, struct stat *buf);
    static libc___xstat_fn *libc___xstat = nullptr;
    if (!libc___xstat) libc___xstat = (libc___xstat_fn*) dlsym(RTLD_NEXT, "__xstat");

    char vpath[PATH_MAX];
    vboxpath(path, vpath, PATH_MAX);

    int rc = libc___xstat(vers, vpath, buf);
    if (rc == 0) return rc;

    return libc___xstat(vers, path, buf);
}

int __xstat64_wrapper(int vers, const char *path, struct stat *buf) {
    logme(fs, "__xstat64(%d, %s)", vers, path);

    typedef int libc___xstat64_fn(int vers, const char *path, struct stat *buf);
    static libc___xstat64_fn *libc___xstat64 = nullptr;
    if (!libc___xstat64) libc___xstat64 = (libc___xstat64_fn*) dlsym(RTLD_NEXT, "__xstat64");

    char vpath[PATH_MAX];
    vboxpath(path, vpath, PATH_MAX);

    int rc = libc___xstat64(vers, vpath, buf);
    if (rc == 0) return rc;

    return libc___xstat64(vers, path, buf);
}

int __lxstat_wrapper(int vers, const char *path, struct stat *buf) {
    logme(fs, "__lxstat(%d, %s)", vers, path);

    typedef int libc___lxstat_fn(int vers, const char *path, struct stat *buf);
    static libc___lxstat_fn *libc___lxstat = nullptr;
    if (!libc___lxstat) libc___lxstat = (libc___lxstat_fn*) dlsym(RTLD_NEXT, "__lxstat");

    char vpath[PATH_MAX];
    vboxpath(path, vpath, PATH_MAX);

    int rc = libc___lxstat(vers, vpath, buf);
    if (rc == 0) return rc;

    return libc___lxstat(vers, path, buf);
}

int __lxstat64_wrapper(int vers, const char *path, struct stat *buf) {
    logme(fs, "__lxstat64(%d, %s)", vers, path);

    typedef int libc___lxstat64_fn(int vers, const char *path, struct stat *buf);
    static libc___lxstat64_fn *libc___lxstat64 = nullptr;
    if (!libc___lxstat64) libc___lxstat64 = (libc___lxstat64_fn*) dlsym(RTLD_NEXT, "__lxstat64");

    char vpath[PATH_MAX];
    vboxpath(path, vpath, PATH_MAX);

    int rc = libc___lxstat64(vers, vpath, buf);
    if (rc == 0) return rc;

    return libc___lxstat64(vers, path, buf);
}


int unlink_wrapper(const char *pathname) {
    logme(fs, "unlink(%s)", pathname);

    typedef int libc_unlink_fn(const char *pathname);
    static libc_unlink_fn *libc_unlink = nullptr;
    if (!libc_unlink) libc_unlink = (libc_unlink_fn*) dlsym(RTLD_NEXT, "unlink");

    char vpath[PATH_MAX];
    vboxpath(pathname, vpath, PATH_MAX);

    int rc = libc_unlink(vpath);
    if (rc == 0) return rc;

    return libc_unlink(pathname);
}

int unlinkat_wrapper(int dirfd, const char *pathname, int flags) {
    logme(fs, "unlinkat(%d, %s)", dirfd, pathname);

    typedef int libc_unlinkat_fn(int dirfd, const char *pathname, int flags);
    static libc_unlinkat_fn *libc_unlinkat = nullptr;
    if (!libc_unlinkat) libc_unlinkat = (libc_unlinkat_fn*) dlsym(RTLD_NEXT, "unlinkat");

    char vpath[PATH_MAX];
    vboxpath(pathname, vpath, PATH_MAX);

    int rc = libc_unlinkat(dirfd, vpath, flags);
    if (rc == 0) return rc;

    return libc_unlinkat(dirfd, pathname, flags);
}


int rename_wrapper(const char *oldpath, const char *newpath) {
    logme(fs, "rename(%s, %s)", oldpath, newpath);

    typedef int libc_rename_fn(const char *oldpath, const char *newpath);
    static libc_rename_fn *libc_rename = nullptr;
    if (!libc_rename) libc_rename = (libc_rename_fn*) dlsym(RTLD_NEXT, "rename");

    char vpath1[PATH_MAX];
    vboxpath(oldpath, vpath1, PATH_MAX);

    char vpath2[PATH_MAX];
    vboxpath(newpath, vpath2, PATH_MAX);

    int rc = libc_rename(vpath1, vpath2);
    if (rc == 0) return rc;

    return libc_rename(oldpath, newpath);
}


int rmdir_wrapper(const char *pathname) {
    logme(fs, "rmdir(%s)", pathname);
    typedef int libc_rmdir_fn(const char *pathname);
    static libc_rmdir_fn *libc_rmdir = nullptr;
    if (!libc_rmdir) libc_rmdir = (libc_rmdir_fn*) dlsym(RTLD_NEXT, "rmdir");

    char vpath[PATH_MAX];
    vboxpath(pathname, vpath, PATH_MAX);

    int rc = libc_rmdir(vpath);
    if (rc == 0) return rc;

    return libc_rmdir(pathname);
}

int mkdir_wrapper(const char *pathname, mode_t mode) {
    logme(fs, "mkdir(%s, %d)", pathname, mode);

    typedef int libc_mkdir_fn(const char *pathname, mode_t mode);
    static libc_mkdir_fn *libc_mkdir = nullptr;
    if (!libc_mkdir) libc_mkdir = (libc_mkdir_fn*) dlsym(RTLD_NEXT, "mkdir");

    char vpath[PATH_MAX];
    vboxpath(pathname, vpath, PATH_MAX);

    int rc = libc_mkdir(vpath, mode);
    if (rc == 0) return rc;

    return libc_mkdir(pathname, mode);
}

int mkdirat_wrapper(int dirfd, const char *pathname, mode_t mode) {
    logme(fs, "mkdirat(%d, %s, %d)", dirfd, pathname, mode);

    typedef int libc_mkdirat_fn(int dirfd, const char *pathname, mode_t mode);
    static libc_mkdirat_fn *libc_mkdirat = nullptr;
    if (!libc_mkdirat) libc_mkdirat = (libc_mkdirat_fn*) dlsym(RTLD_NEXT, "mkdirat");

    char vpath[PATH_MAX];
    vboxpath(pathname, vpath, PATH_MAX);

    int rc = libc_mkdirat(dirfd, vpath, mode);
    if (rc == 0) return rc;

    return libc_mkdirat(dirfd, pathname, mode);
}

int chdir_wrapper(const char *pathname) {
    logme(fs, "chdir(%s)", pathname);

    typedef int libc_chdir_fn(const char *pathname);
    static libc_chdir_fn *libc_chdir = nullptr;
    if (!libc_chdir) libc_chdir = (libc_chdir_fn*) dlsym(RTLD_NEXT, "chdir");

    char vpath[PATH_MAX];
    vboxpath(pathname, vpath, PATH_MAX);

    int rc = libc_chdir(vpath);
    if (rc == 0) return rc;

    return libc_chdir(pathname);
}


ssize_t readlink_wrapper(const char *pathname, char *buf, size_t bufsiz) {
    logme(fs, "readlink(%s)", pathname);

    // called by dlsym() and locks in malloc
    if (strcmp(pathname, "/etc/malloc.conf") == 0)
        return -1;

    typedef ssize_t libc_readlink_fn(const char *pathname, char *buf, size_t bufsiz);
    static libc_readlink_fn *libc_readlink = nullptr;
    if (!libc_readlink) libc_readlink = (libc_readlink_fn*) dlsym(RTLD_NEXT, "readlink");

    char vpath[PATH_MAX];
    vboxpath(pathname, vpath, PATH_MAX);

    ssize_t rc = libc_readlink(pathname, buf, bufsiz);
    if (rc >= 0) return rc;

    return libc_readlink(pathname, buf, bufsiz);
}


char *realpath_wrapper(const char *path, char *resolved_path) {
    logme(fs, "realpath(%s)", path);

    typedef char *libc_realpath_fn(const char *path, char *resolved_path);
    static libc_realpath_fn *libc_realpath = nullptr;
    if (!libc_realpath) libc_realpath = (libc_realpath_fn*) dlsym(RTLD_NEXT, "realpath");

    char vpath[PATH_MAX];
    vboxpath(path, vpath, PATH_MAX);

    char *rc = libc_realpath(vpath, resolved_path);
    if (rc) return rc;

    return libc_realpath(path, resolved_path);
}


int chmod_wrapper(const char *pathname, mode_t mode) {
    logme(fs, "chmod(%s, %d)", pathname, mode);

    typedef int libc_chmod_fn(const char *pathname, mode_t mode);
    static libc_chmod_fn *libc_chmod = nullptr;
    if (!libc_chmod) libc_chmod = (libc_chmod_fn*) dlsym(RTLD_NEXT, "chmod");

    char vpath[PATH_MAX];
    vboxpath(pathname, vpath, PATH_MAX);

    int rc = libc_chmod(vpath, mode);
    if (rc == 0) return rc;

    return libc_chmod(pathname, mode);
}

int fchmodat_wrapper(int dirfd, const char *pathname, mode_t mode, int flags) {
    logme(fs, "fchmodat(%s, %d, %d)", pathname, mode, flags);

    typedef int libc_fchmodat_fn(int dirfd,const char *pathname, mode_t mode, int flags);
    static libc_fchmodat_fn *libc_fchmodat = nullptr;
    if (!libc_fchmodat) libc_fchmodat = (libc_fchmodat_fn*) dlsym(RTLD_NEXT, "fchmodat");

    char vpath[PATH_MAX];
    vboxpath(pathname, vpath, PATH_MAX);

    int rc = libc_fchmodat(dirfd, vpath, mode, flags);
    if (rc == 0) return rc;

    return libc_fchmodat(dirfd, pathname, mode, flags);
}


int access_wrapper(const char *pathname, int mode) {
    logme(fs, "access(%s, %d)", pathname, mode);

    typedef int libc_access_fn(const char *pathname, mode_t mode);
    static libc_access_fn *libc_access = nullptr;
    if (!libc_access) libc_access = (libc_access_fn*) dlsym(RTLD_NEXT, "access");

    char vpath[PATH_MAX];
    vboxpath(pathname, vpath, PATH_MAX);

    int rc = libc_access(vpath, mode);
    if (rc == 0) return rc;

    return libc_access(pathname, mode);
}

int faccessat_wrapper(int dirfd, const char *pathname, int mode, int flags) {
    logme(fs, "faccessat(%s, %d)", pathname, mode);

    typedef int libc_faccessat_fn(int dirfd,const char *pathname, mode_t mode, int flags);
    static libc_faccessat_fn *libc_faccessat = nullptr;
    if (!libc_faccessat) libc_faccessat = (libc_faccessat_fn*) dlsym(RTLD_NEXT, "faccessat");

    char vpath[PATH_MAX];
    vboxpath(pathname, vpath, PATH_MAX);

    int rc = libc_faccessat(dirfd, vpath, mode, flags);
    if (rc == 0) return rc;

    return libc_faccessat(dirfd, pathname, mode, flags);
}

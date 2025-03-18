#include "vbox_fs_wrapper.h"

extern "C" DIR *opendir(const char *name) {
    return opendir_wrapper(name);
}


extern "C" int open(const char *pathname, int flags, int mode) {
    return open_wrapper(pathname, flags, mode);
}


extern "C" int open64(const char *pathname, int flags, int mode) {
    return open_wrapper(pathname, flags, mode);
}


extern "C" FILE *fopen(const char *path, const char *mode) {
    return fopen_wrapper(path, mode);
}

extern "C" FILE *fopen64(const char *path, const char *mode) {
    return fopen64_wrapper(path, mode);
}


extern "C" int link(const char *oldpath, const char *newpath) {
    return link_wrapper(oldpath, newpath);
}


extern "C" int symlink(const char *oldpath, const char *newpath) {
    return symlink_wrapper(oldpath, newpath);
}


struct stat;

extern "C" int __xstat(int vers, const char *path, struct stat *buf) {
    return __xstat_wrapper(vers, path, buf);
}

extern "C" int __xstat64(int vers, const char *path, struct stat *buf) {
    return __xstat64_wrapper(vers, path, buf);
}

extern "C" int __lxstat(int vers, const char *path, struct stat *buf) {
    return __lxstat_wrapper(vers, path, buf);
}

extern "C" int __lxstat64(int vers, const char *path, struct stat *buf) {
    return __lxstat64_wrapper(vers, path, buf);
}


extern "C" int unlink(const char *pathname) {
    return unlink_wrapper(pathname);
}

extern "C" int unlinkat(int dirfd, const char *pathname, int flags) {
    return unlinkat_wrapper(dirfd, pathname, flags);
}


extern "C" int rename(const char *oldpath, const char *newpath) {
    return rename_wrapper(oldpath, newpath);
}


extern "C" int rmdir(const char *pathname) {
    return rmdir_wrapper(pathname);
}

extern "C" int mkdir(const char *pathname, mode_t mode) {
    return mkdir_wrapper(pathname, mode);
}

extern "C" int mkdirat(int dirfd, const char *pathname, mode_t mode) {
    return mkdirat_wrapper(dirfd, pathname, mode);
}

extern "C" int chdir(const char *pathname) {
    return chdir_wrapper(pathname);
}


extern "C" ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
    return readlink_wrapper(pathname, buf, bufsiz);
}


extern "C" char *realpath(const char *path, char *resolved_path) {
    return realpath_wrapper(path, resolved_path);
}


extern "C" int chmod(const char *pathname, mode_t mode) {
    return chmod_wrapper(pathname, mode);
}

extern "C" int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) {
    return fchmodat_wrapper(dirfd, pathname, mode, flags);
}


extern "C" int access(const char *pathname, int mode) {
    return access_wrapper(pathname, mode);
}

extern "C" int faccessat(int dirfd, const char *pathname, int mode, int flags) {
    return faccessat_wrapper(dirfd, pathname, mode, flags);
}

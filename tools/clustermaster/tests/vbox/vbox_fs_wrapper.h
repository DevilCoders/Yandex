#pragma once

#include <sys/types.h>

extern "C" {

typedef void *DIR;

DIR *opendir_wrapper(const char *name);
int open_wrapper(const char *pathname, int flags, ...);
int open64_wrapper(const char *pathname, int flags, ...);

typedef struct _IO_FILE FILE;
FILE *fopen_wrapper(const char *path, const char *mode);
FILE *fopen64_wrapper(const char *path, const char *mode);

int link_wrapper(const char *oldpath, const char *newpath);
int symlink_wrapper(const char *oldpath, const char *newpath);

struct stat;
int __xstat_wrapper(int vers, const char *path, struct stat *buf);
int __xstat64_wrapper(int vers, const char *path, struct stat *buf);
int __lxstat_wrapper(int vers, const char *path, struct stat *buf);
int __lxstat64_wrapper(int vers, const char *path, struct stat *buf);

int unlink_wrapper(const char *pathname);
int unlinkat_wrapper(int dirfd, const char *pathname, int flags);

int rename_wrapper(const char *oldpath, const char *newpath);

int rmdir_wrapper(const char *pathname);
int mkdir_wrapper(const char *pathname, mode_t mode);
int mkdirat_wrapper(int dirfd, const char *pathname, mode_t mode);
int chdir_wrapper(const char *pathname);

ssize_t readlink_wrapper(const char *pathname, char *buf, size_t bufsiz);
char *realpath_wrapper(const char *path, char *resolved_path);

int chmod_wrapper(const char *pathname, mode_t mode);
int fchmodat_wrapper(int dirfd, const char *pathname, mode_t mode, int flags);

int access_wrapper(const char *pathname, int mode);
int faccessat_wrapper(int dirfd, const char *pathname, int mode, int flags);

}

#include <util/system/mutex.h>

//hack: open <> open64

#include <features.h>

#undef __USE_FILE_OFFSET64
#undef __USE_LARGEFILE64

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/guard.h>

#include <dlfcn.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>

extern FILE *flog;
#define LOGME 0

#include <errno.h>

// Originals

#include <dirent.h>

typedef DIR *opendir_fn(const char *name);
static opendir_fn *_opendir = 0;


#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>

typedef int openat_fn(int dirfd, const char *pathname, int flags, ...);
static openat_fn *_openat = 0;
static openat_fn *_openat64 = 0;
typedef int open_fn(const char *pathname,int flags, ...);
static open_fn *_open = 0;
static open_fn *_open64 = 0;
typedef int creat_fn(const char *pathname, mode_t mode);
static creat_fn *_creat = 0;

typedef FILE *fopen_fn(const char *path, const char *mode);
static fopen_fn *_fopen = 0;
static fopen_fn *_fopen64 = 0;


#include <unistd.h>

typedef int link_fn(const char *oldpath, const char *newpath);
static link_fn *_link = 0;

#include <unistd.h>

typedef int symlink_fn(const char *oldpath, const char *newpath);
static symlink_fn *_symlink = 0;

#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>

typedef int stat_fn(int vers, const char *path, struct stat *buf);
static stat_fn *_stat = 0;
static stat_fn *_stat64 = 0;
typedef int lstat_fn(int vers, const char *path, struct stat *buf);
static lstat_fn *_lstat = 0;
static lstat_fn *_lstat64 = 0;

#include <unistd.h>

typedef int unlink_fn(const char *pathname);
static unlink_fn *_unlink = 0;
typedef int unlinkat_fn(int dirfd, const char *pathname, int flags);
static unlinkat_fn *_unlinkat = 0;

#include <stdio.h>

typedef int rename_fn(const char *oldpath, const char *newpath);
static rename_fn *_rename = 0;

#include <unistd.h>

typedef int rmdir_fn(const char *pathname);
static rmdir_fn *_rmdir = 0;

#include <sys/stat.h>
#include <sys/types.h>

typedef int mkdir_fn(const char *pathname, mode_t mode);
static mkdir_fn *_mkdir = 0;

#include <unistd.h>

typedef int chdir_fn(const char *path);
static chdir_fn *_chdir = 0;

typedef ssize_t readlink_fn(const char *pathname, char *buf, size_t bufsiz);
static readlink_fn *O_readlink = 0;

//level-3

#include <limits.h>
#include <stdlib.h>

typedef char *realpath_fn(const char *path, char *resolved_path);
static realpath_fn *_realpath = 0;


__attribute__((constructor)) static void fs_initialize() {
static int init = 0;
if (init > 0) return ;
//if(LOGME)fprintf(stderr,"fs initialize(%d) %s\n",init++,"XXX"/*getprogname()*/);
    _opendir    = (opendir_fn*)      dlsym(RTLD_NEXT, "opendir");
    _openat  = (openat_fn*)    dlsym(RTLD_NEXT, "__openat_2");
    _openat64  = (openat_fn*)    dlsym(RTLD_NEXT, "__openat64_2");
    _fopen    = (fopen_fn*)      dlsym(RTLD_NEXT, "fopen");
    _fopen64    = (fopen_fn*)      dlsym(RTLD_NEXT, "fopen64");
    _open    = (open_fn*)      dlsym(RTLD_NEXT, "open");
    _open64  = (open_fn*)      dlsym(RTLD_NEXT, "open64");
    _creat   = (creat_fn*)     dlsym(RTLD_NEXT, "creat");
    _link    = (link_fn*)      dlsym(RTLD_NEXT, "link");
    _symlink = (symlink_fn*)   dlsym(RTLD_NEXT, "symlink");
    _stat    = (stat_fn*)      dlsym(RTLD_NEXT, "__xstat");
    _lstat   = (lstat_fn*)     dlsym(RTLD_NEXT, "__lxstat");
    _stat64    = (stat_fn*)      dlsym(RTLD_NEXT, "__xstat64");
    _lstat64   = (lstat_fn*)     dlsym(RTLD_NEXT, "__lxstat64");
    _unlink  = (unlink_fn*)    dlsym(RTLD_NEXT, "unlink");
    _unlinkat  = (unlinkat_fn*)    dlsym(RTLD_NEXT, "unlinkat");
    _rename  = (rename_fn*)    dlsym(RTLD_NEXT, "rename");
    _rmdir   = (rmdir_fn*)     dlsym(RTLD_NEXT, "unlink");
    _mkdir   = (mkdir_fn*)     dlsym(RTLD_NEXT, "mkdir");
    _chdir   = (chdir_fn*)     dlsym(RTLD_NEXT, "chdir");
    O_readlink   = (readlink_fn*)     dlsym(RTLD_NEXT, "readlink");
    _realpath   = (realpath_fn*)     dlsym(RTLD_NEXT, "realpath");

    //flog = fopen("/tmp/vbox.log","a+");
}


extern "C" DIR *opendir(const char *name) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"opendir(%s)\n",name);
    if (*name=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), name);
        DIR *dd = _opendir(s);
if(LOGME)fprintf(flog,"#opendir(%s) = %p\n",s,dd);
        if (dd) return dd;
    }
    DIR *dd = _opendir(name);
if(LOGME)fprintf(flog,"@opendir(%s) = %p\n",name,dd);
    return dd;
}

#if 0
extern "C" int openat(int dirfd, const char *pathname, int flags, ...)  {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"openat(%d, %s, %d)\n",dirfd,pathname,flags);
    return _openat(dirfd,pathname,flags);
}
extern "C" int openat64(int dirfd, const char *pathname, int flags, ...)  {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"openat(%d, %s, %d)\n",dirfd,pathname,flags);
    return _openat(dirfd,pathname,flags);
}

int __openat_2(int dirfd, const char *pathname, int flags, ...)  {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"__openat_2(%d, %s, %d)\n",dirfd,pathname,flags);
    return _openat(dirfd,pathname,flags);
}

extern "C" int __openat64_2(int dirfd, const char *pathname, int flags, ...) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"__openat64_2(%d, %s, %d)\n",dirfd,pathname,flags);
    return _openat64(dirfd,pathname,flags);
}
#endif
#if 0
extern "C" int __open(const char * /*pathname*/, int /*flags*/, ...) {
TMutex mx; TGuard<TMutex> lock(mx);
    abort(); return 0;
}
extern "C" int __open64(const char * /*pathname*/, int /*flags*/, ...) {
TMutex mx; TGuard<TMutex> lock(mx);
    abort(); return 0;
}
extern "C" int __open_2(const char * /*pathname*/, int /*flags*/, ...) {
TMutex mx; TGuard<TMutex> lock(mx);
    abort(); return 0;
}
extern "C" int __open64_2(const char * /*pathname*/, int /*flags*/, ...) {
TMutex mx; TGuard<TMutex> lock(mx);
    abort(); return 0;
}
#endif

extern "C" FILE *fopen(const char *path, const char *mode) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"fopen(%s, %s)\n",path,mode);
if (!_fopen) {
    _fopen = (fopen_fn*) dlsym(RTLD_NEXT, "fopen");
    fprintf(stderr, "!!! _fopen=%p\n", _fopen);
}
    if (*path=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), path);
        FILE *fp = _fopen(s, mode);
if(LOGME)fprintf(flog,"#fopen(%s, %s) = %p\n",s,mode,fp);
        if (fp) return fp;
    }
    FILE *fp = _fopen(path, mode);
if(LOGME)fprintf(flog,"@fopen(%s, %s) = %p\n",path,mode,fp);
    return fp;
}

extern "C" FILE *fopen64(const char *path, const char *mode) {
//TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"fopen64(%s, %s)\n",path,mode);
if (!_fopen64) {
    _fopen64 = (fopen_fn*) dlsym(RTLD_NEXT, "fopen64");
    fprintf(stderr, "!!! _fopen64=%p\n", _fopen64);
}
    if (*path=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), path);
        FILE *fp = _fopen64(s, mode);
if(LOGME)fprintf(flog,"#fopen64(%s, %s) = %p\n",s,mode,fp);
        if (fp) return fp;
    }
    FILE *fp = _fopen64(path, mode);
if(LOGME)fprintf(flog,"@fopen64(%s, %s) = %p\n",path,mode,fp);
    return fp;
}

extern "C" int open(const char *pathname, int flags, ...) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"open(%s, %d)\n",pathname,flags);
    mode_t mode = 0;
if (flags | O_CREAT) {
    va_list ap;
    va_start(ap,flags);
    mode = va_arg(ap,mode_t);
    if(LOGME)fprintf(flog,"mode=%d\n",mode);
    va_end(ap);
}
    if (*pathname=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), pathname);
        int fd = _open(s, flags, mode);
if(LOGME)fprintf(flog,"#open(%s, %d, %d) = %d\n",s,flags,mode,fd);
        if (fd > 0) return fd;
    }
    int fd = _open(pathname, flags, mode);
if(LOGME)fprintf(flog,"@open(%s, %d, %d) = %d\n",pathname,flags,mode,fd);
    return fd;
}
extern "C" int open64(const char *pathname, int flags, ...) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"open64(%s, %d)\n",pathname,flags);
    mode_t mode = 0;
if (flags | O_CREAT) {
    va_list ap;
    va_start(ap,flags);
    mode = va_arg(ap,mode_t);
    if(LOGME)fprintf(flog,"mode=%d\n",mode);
    va_end(ap);
}
    if (*pathname=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), pathname);
        int fd = _open64(s, flags, mode);
if(LOGME)fprintf(flog,"#open64(%s, %d, %d) = %d\n",s,flags,mode,fd);
        if (fd > 0) return fd;
    }
    int fd = _open64(pathname, flags, mode);
if(LOGME)fprintf(flog,"@open64(%s, %d, %d) = %d\n",pathname,flags,mode,fd);
    return fd;
}
extern "C" int creat(const char *pathname, mode_t mode) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"creat(%s, %d)\n",pathname,mode);
    if (*pathname=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), pathname);
        return _creat(s, mode);
    }
    return _creat(pathname, mode);
}

extern "C" int link(const char *oldpath, const char *newpath) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"link(%s, %s)\n",oldpath,newpath);
    char s1[4096]={0};
    char s2[4096]={0};
    if (*oldpath=='/') {
        sprintf(s1, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), oldpath);
        oldpath = s1;
    }
    if (*newpath=='/') {
        sprintf(s2, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), newpath);
        newpath = s2;
    }
if(LOGME)fprintf(flog,"#link(%s, %s)\n",oldpath,newpath);
    return _link(oldpath, newpath);
}

extern "C" int symlink(const char *oldpath, const char *newpath) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"symlink(%s, %s)\n",oldpath,newpath);
    char s1[4096]={0};
    char s2[4096]={0};
    if (*oldpath=='/') {
        sprintf(s1, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), oldpath);
        oldpath = s1;
    }
    if (*newpath=='/') {
        sprintf(s2, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), newpath);
        newpath = s2;
    }
if(LOGME)fprintf(flog,"#symlink(%s, %s)\n",oldpath,newpath);
    return _symlink(oldpath, newpath);
}

extern "C" int __xstat(int vers, const char *path, struct stat *buf) {
TMutex mx; TGuard<TMutex> lock(mx);
//if(LOGME)fprintf(flog,"stat(%s, %p)\n",path,buf);
    if (*path=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), path);
        int rc = _stat(vers, s, buf);
//if(LOGME)fprintf(flog,"#stat(%s, %p) = %d\n",s,buf,rc);
        if (rc == 0) return rc;
    }
    int rc = _stat(vers, path, buf);
//if(LOGME)fprintf(flog,"@stat(%s, %p) = %d\n",path,buf,rc);
    return rc;
}
extern "C" int __xstat64(int vers, const char *path, struct stat *buf) {
TMutex mx; TGuard<TMutex> lock(mx);
//if(LOGME)fprintf(flog,"stat64(%s, %p)\n",path,buf);
    if (*path=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), path);
        int rc = _stat64(vers, s, buf);
//if(LOGME)fprintf(flog,"#stat64(%s, %p) = %d\n",s,buf,rc);
        if (rc == 0) return rc;
    }
    int rc = _stat64(vers, path, buf);
//if(LOGME)fprintf(flog,"@stat64(%s, %p) = %d\n",path,buf,rc);
    return rc;
}
extern "C" int __lxstat(int vers, const char *path, struct stat *buf) {
TMutex mx; TGuard<TMutex> lock(mx);
//if(LOGME)fprintf(flog,"lstat(%s, %p)\n",path,buf);
    if (*path=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), path);
        int rc = _lstat(vers, s, buf);
        if (rc == 0) return rc;
    }
    return _lstat(vers, path, buf);
}
extern "C" int __lxstat64(int vers, const char *path, struct stat *buf) {
TMutex mx; TGuard<TMutex> lock(mx);
//if(LOGME)fprintf(flog,"lstat64(%s, %p)\n",path,buf);
    if (*path=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), path);
        int rc = _lstat64(vers, s, buf);
        if (rc == 0) return rc;
    }
    return _lstat64(vers, path, buf);
}

extern "C" int unlink(const char *pathname) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"unlink(%s)\n",pathname);
    if (*pathname=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), pathname);
        int rc = _unlink(s);
if(LOGME)fprintf(flog,"#unlink(%s) = %d\n",s,rc);
        if (rc == 0) return rc;
    }
    int rc = _unlink(pathname);
if(LOGME)fprintf(flog,"@unlink(%s) = %d\n",pathname,rc);
    return rc;
}

extern "C" int unlinkat(int dirfd, const char *pathname, int flags) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"unlinkat(%s, %d)\n",pathname,flags);
    if (*pathname=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), pathname);
        int rc = _unlinkat(dirfd, s, flags);
if(LOGME)fprintf(flog,"#unlinkat(%s, %d) = %d\n",s,flags,rc);
        if (rc == 0) return rc;
    }
    int rc = _unlinkat(dirfd, pathname, flags);
if(LOGME)fprintf(flog,"@unlinkat(%s, %d) = %d\n",pathname,flags,rc);
    return rc;
}

extern "C" int rename(const char *oldpath, const char *newpath) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"rename(%s, %s)\n",oldpath,newpath);
    char s1[4096]={0};
    char s2[4096]={0};
    if (*oldpath=='/') {
        sprintf(s1, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), oldpath);
        oldpath = s1;
    }
    if (*newpath=='/') {
        sprintf(s2, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), newpath);
        newpath = s2;
    }
if(LOGME)fprintf(flog,"#rename(%s, %s)\n",oldpath,newpath);
    return _rename(oldpath, newpath);
}

extern "C" int rmdir(const char *pathname) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"rmdir(%s)\n",pathname);
    if (*pathname=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), pathname);
        int rc =  _rmdir(s);
if(LOGME)fprintf(flog,"#rmdir(%s) = %d\n",s,rc);
        if (rc == 0) return rc;
    }
    int rc = _rmdir(pathname);
if(LOGME)fprintf(flog,"@rmdir(%s) = %d\n",pathname,rc);
    return rc;
}

extern "C" int mkdir(const char *pathname, mode_t mode) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"mkdir(%s, %d)\n",pathname,mode);
    if (*pathname=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), pathname);
        int rc = _mkdir(s, mode);
if(LOGME)fprintf(flog,"#mkdir(%s, %d) = %d\n",s,mode,rc);
        if (rc == 0 || errno == EEXIST) return rc;
    }
    int rc = _mkdir(pathname, mode);
if(LOGME)fprintf(flog,"@mkdir(%s, %d) = %d\n",pathname,mode,rc);
    return rc;
}

extern "C" int chdir(const char *path) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"chdir(%s)\n",path);
    if (*path=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), path);
if(LOGME)fprintf(flog,"#chdir(%s)\n",s);
        return _chdir(s);
    }
    return _chdir(path);
}


extern "C" ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
//if(LOGME)fprintf(stderr,"readlink(%s)\n",pathname);
//dlsym() in malloc calls readlink() so we have deadlock
if (strcmp(pathname, "/etc/malloc.conf") == 0) return -1;

    if (!O_readlink)
        O_readlink = (readlink_fn*) dlsym(RTLD_NEXT,"readlink");

    if (*pathname=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), pathname);
//if(LOGME)fprintf(stderr,"#readlink(%s)\n",s);
        ssize_t n = O_readlink(s, buf, bufsiz);
//if(LOGME)fprintf(stderr,"n=%lu\n",n);
        if (n >= 0) return n;
    }

    return O_readlink(pathname, buf, bufsiz);
}

extern "C" char *realpath(const char *path, char *resolved_path) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"realpath(%s,%s)\n",path,resolved_path);

    if (resolved_path == NULL) {
        resolved_path = (char*) malloc(PATH_MAX);
    }
    if (*path == '/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", ~GetEnv("VBOX_ROOT"), ~GetEnv("HOSTNAME"), path);
        char *p = _realpath(s, resolved_path);
if(LOGME)fprintf(flog,"#realpath(%s,%s)\n",s,resolved_path);
        if (p) return p;
    }
    char *p = _realpath(path, resolved_path);
if(LOGME)fprintf(flog,"@realpath(%s,%s)\n",path,resolved_path);
    return p;
}

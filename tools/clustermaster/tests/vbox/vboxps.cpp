#undef fprintf

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/system/env.h>

#include <dlfcn.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>

extern FILE *flog;
#define LOGME 0

// Originals

#include <unistd.h>

#if 0
/* frontends */
typedef int execl_fn(const char *path, const char *arg, ... /* (char  *) NULL */);
static execl_fn *_execl = 0;
typedef int execlp_fn(const char *file, const char *arg, ... /* (char  *) NULL */);
static execlp_fn *_execlp = 0;
typedef int execle_fn(const char *path, const char *arg, ... /*, (char *) NULL, char * const envp[] */);
static execle_fn *_execle = 0;
typedef int execv_fn(const char *path, char *const argv[]);
static execv_fn *_execv = 0;
typedef int execvp_fn(const char *file, char *const argv[]);
static execvp_fn *_execvp = 0;
typedef int execvpe_fn(const char *file, char *const argv[], char *const envp[]);
static execvpe_fn *_execvpe = 0;
#endif
typedef int execve_fn(const char *filename, char *const argv[], char *const envp[]);
static execve_fn *_execve = nullptr;


//Helpers

__attribute__((constructor)) static void ps_initialize() {
static int init = 0;
if (init > 0) return;
//if(LOGME)fprintf(flog,"ps initialize(%d) %s\n",init++,getprogname());
    _execve = (execve_fn*) dlsym(RTLD_NEXT, "execve");

    //flog = fopen("/tmp/vbox.log","a+");
}


//Wrappers impl
extern "C" int execve(const char *filename, char *const argv[], char *const envp[]) {
if(LOGME)fprintf(flog,"execve(%s)\n",filename);
    if (*filename=='/') {
        char s[4096]={0};
        sprintf(s, "%s/%s/%s", GetEnv("VBOX_ROOT").data(), GetEnv("HOSTNAME").data(), filename);
if(LOGME)fprintf(flog,"#execve(%s)\n",s);
        _execve(s, argv, envp);
    }
if(LOGME)fprintf(flog,"@execve(%s)\n",filename);
    return _execve(filename, argv, envp);
}

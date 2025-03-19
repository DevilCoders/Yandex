#include "urlnorm.h"
#include <cstdio>


void GetHostCacheName(MD5 &HostSignature, const char *Host, ui8 scheme)
{
    HostSignature.Init();
    if (Host) {
        char host[FILENAME_MAX+1];
        NormalizeHostName(host, Host, FILENAME_MAX);
        HostSignature.Update((const unsigned char*)host, strlen(host));
        if (scheme != THttpURL::SchemeHTTP)
            HostSignature.Update((const unsigned char*)&scheme, sizeof(scheme));
    }
}

void GetPathCacheName(char Cache[/*FILENAME_MAX*/], MD5 &HostSignature, const char *Path, int sparse)
{
    if (Path) {
        char path[FILENAME_MAX+1];
        NormalizeUrlName(path, Path, FILENAME_MAX);
        HostSignature.Update((const unsigned char*)path, strlen(path));
    }
    HostSignature.End(Cache);
    if (sparse) {
        memmove(Cache+3, Cache+2, 31);
        memmove(Cache+6, Cache+5, 29);
        Cache[2] = Cache[5] = LOCSLASH_C;
    }
}

void GetUrlCacheName(char Cache[/*FILENAME_MAX*/], const char *Url, int sparse)
{
    THttpURL url;
    MD5 sig;

    if (Url)
        url.Parse(Url);
    GetHostCacheName(sig, url.PrintS(THttpURL::FlagHostPort).data());
    TString path = url.PrintS(THttpURL::FlagPath|THttpURL::FlagQuery);
    GetPathCacheName(Cache, sig, path.data(), sparse);
}

#ifdef _TEST_

int main(int argc, char *argv[], char *[])
{
    char cache[FILENAME_MAX+20];
    if (argc == 1 || argc == 2 && !strcmp(argv[1], "-")) {
        char HostName[FILENAME_MAX+20];
        while(fgets(HostName, FILENAME_MAX, stdin)) {
            char *ptr = strchr(HostName, '\r');
            if (ptr) *ptr = 0;
            ptr = strchr(HostName, '\n');
            if (ptr) *ptr = 0;
            GetUrlCacheName(cache, HostName);
            puts(cache);
        }
    }
    else {
        for(int i = 1; i < argc; i++) {
            GetUrlCacheName(cache, argv[i]);
            puts(cache);
        }
    }
    return 0;
}

#endif

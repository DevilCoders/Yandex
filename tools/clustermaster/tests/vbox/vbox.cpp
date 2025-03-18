#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/system/mutex.h>

#include <dlfcn.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility>

FILE *flog = stderr;
#define LOGME 0

// Originals

#include <sys/utsname.h>

#ifdef FREEBSD
typedef int __xuname_fn(int, void *);
static __xuname_fn *_xuname = 0;
#else
typedef int uname_fn(struct utsname *buf);
static uname_fn *_uname = 0;
#endif

#include <unistd.h>

typedef int gethostname_fn(char *name, size_t len);
static gethostname_fn *_gethostname = 0;

#include <netdb.h>

typedef struct hostent *gethostbyname_fn(const char *name);
static gethostbyname_fn *_gethostbyname = 0;

#include <sys/socket.h>
#include <sys/types.h>

typedef int getaddrinfo_fn(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
static getaddrinfo_fn *_getaddrinfo = 0;

#include <arpa/inet.h>

typedef int inet_pton_fn(int af, const char *src, void *dst);
static inet_pton_fn *_inet_pton = 0;

#include <netinet/in.h>

typedef int connect_fn(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
static connect_fn *_connect = 0;
typedef int bind_fn(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
static bind_fn *_bind = 0;


//Helpers
static char thishost[256] = {0};
//static struct addrinfo *thisaddr;
//static THashMap<TString,int> vhosts;
static std::map<TString,int> vhosts;

__attribute__((constructor)) static void initialize() {
static int init = 0;
if (init > 0) return;
//if(LOGME)fprintf(flog,"net initialize(%d) %s\n",init++,getprogname());
#ifdef FREEBSD
    _xuname = (__xuname_fn*) dlsym(RTLD_NEXT, "__xuname");
#else
    _uname  = (uname_fn*) dlsym(RTLD_NEXT, "uname");
#endif
    _gethostname   = (gethostname_fn*)   dlsym(RTLD_NEXT, "gethostname");
    _gethostbyname = (gethostbyname_fn*) dlsym(RTLD_NEXT, "gethostbyname");
    _getaddrinfo   = (getaddrinfo_fn*)   dlsym(RTLD_NEXT, "getaddrinfo");
    _inet_pton     = (inet_pton_fn*)     dlsym(RTLD_NEXT, "inet_pton");

    _bind    = (bind_fn*)    dlsym(RTLD_NEXT, "bind");
    _connect = (connect_fn*) dlsym(RTLD_NEXT, "connect");

    _gethostname(thishost, sizeof(thishost));

    //flog = fopen("/tmp/vbox.log","a+");
}

int getAddrOfHost(const char *name) {
    if (vhosts.empty()) {
        TUnbufferedFileInput in(GetEnv("VBOX_HOSTS"));
        TString vhost;
        uint32_t addr = 0xC0A80101UL;

        while(in.ReadLine(vhost)) {
if(LOGME)fprintf(flog,"%s\n",~vhost);
            vhosts.insert(std::make_pair(vhost, addr));
            addr += 256;
        }
    }
    auto it = vhosts.find(name);
    return (it == vhosts.end()) ? 0 : it->second;
}

const char *getNameOfHost(const char *name) {
    if (vhosts.empty()) {
        getAddrOfHost(name);
    }
    auto it = vhosts.find(name);
    return (it == vhosts.end()) ? 0 : it->first.c_str();
}

uint16_t getBoundPort(int family, uint32_t addr, uint16_t port) {
    char fname[PATH_MAX];
    sprintf(fname, "%s/vboxhosts.map", ~GetEnv("VBOX_ROOT"));
    FILE *in = fopen(fname, "r");
    if (!in) return 0;
    flock(fileno(in), LOCK_SH);

    while (!feof(in)) {
        int sfamily = 0;
        uint32_t saddr = 0;
        uint16_t sport = 0;
        uint16_t dport = 0;
        fread(&sfamily, 1, 4, in);
        fread(&saddr, 1, 4, in);
        fread(&sport, 1, 2, in);
        fread(&dport, 1, 2, in);
if(LOGME)fprintf(flog,"getBoundPort(): saddr=%#x sport=%d dport=%d\n",ntohl(saddr),ntohs(sport),ntohs(dport));
        if (family == sfamily && addr == saddr && port == sport) {
if(LOGME)fprintf(flog,"bingo!\n");
            flock(fileno(in), LOCK_UN);
            fclose(in);
            return dport;
        }
    }
    flock(fileno(in), LOCK_UN);
    fclose(in);
    return 0;
}

void putBoundPort(int family, uint32_t addr, uint16_t sport, uint16_t dport) {
    uint16_t port = getBoundPort(family, addr, sport);
    if (port > 0) return;

    char fname[PATH_MAX];
    sprintf(fname, "%s/vboxhosts.map", ~GetEnv("VBOX_ROOT"));
    FILE *out = fopen(fname, "a+");
    flock(fileno(out), LOCK_EX);
    fwrite(&family, 1, 4, out);
    fwrite(&addr, 1, 4, out);
    fwrite(&sport, 1, 2, out);
    fwrite(&dport, 1, 2, out);

    flock(fileno(out), LOCK_UN);
    fclose(out);
}

//Wrappers impl
#ifdef FREEBSD
extern "C" int __xuname(int len, void *buf) {
TMutex mx; TGuard<TMutex> lock(mx);
    int rc = _xuname(len, buf);
    strncpy(((struct utsname *)buf)->nodename, ~GetEnv("HOSTNAME"), len);
    return rc;
}
#else
extern "C" int uname(struct utsname *buf) {
TMutex mx; TGuard<TMutex> lock(mx);
    int rc = _uname(buf);
    strncpy(buf->nodename, ~GetEnv("HOSTNAME"), sizeof(buf->nodename));
    return rc;
}
#endif


extern "C" int gethostname(char *name, size_t len) {
TMutex mx; TGuard<TMutex> lock(mx);
    strncpy(name, ~GetEnv("HOSTNAME"), len);
    return 0;
}


extern "C" struct hostent *gethostbyname(const char *name) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"gethostbyname %s\n",name);

    int addr = getAddrOfHost(name);
    if (addr == 0) {
        return _gethostbyname(name);
    }

    static struct hostent *host = 0;
    if (!host) {
        host = _gethostbyname(thishost);
        switch (host->h_addrtype) {
        case AF_INET:;
        case AF_INET6:;
        }
        memcpy(host->h_addr, &addr, host->h_length);
    }

    return host;
}


extern "C" int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
//TMutex mx; TGuard<TMutex> lock(mx);

if(LOGME)fprintf(flog,"getaddrinfo(%s, %s) -- thishost=%s\n",node,service,thishost);
    ((struct addrinfo*)hints)->ai_family = AF_INET6; // CM fails with both

    //in iet_pton e have list of ifaddrs
    if (node && !strcmp(node, "0.0.0.0")) node = "twalrus4";

    int addr = getAddrOfHost(node ? node : ~GetEnv("HOSTNAME"));
    if (addr == 0) {
if(LOGME)fprintf(flog,"getaddrinfo(%s, %s) fake addr NOT found\n",node,service);
        return _getaddrinfo(node, service, hints, res);
    }

    int rc = _getaddrinfo(thishost, service, hints, res);
if(LOGME)fprintf(flog,"getaddrinfo(%s, %s) fake addr %#x FOUND! = %d\n",thishost,service,addr,rc);
    addr = htonl(addr);
    for (struct addrinfo *r = *res; r != 0; r = r->ai_next) {
        switch (r->ai_family) {
        case AF_INET: {
            struct sockaddr_in *in4 = (struct sockaddr_in *) r->ai_addr;
if(LOGME)fprintf(flog,"getaddrinfo() patching ipv4\n");
            memcpy(&in4->sin_addr, &addr, r->ai_addrlen);
        }
        break;
        case AF_INET6: {
            struct sockaddr_in6 *in6 = (struct sockaddr_in6 *) r->ai_addr;
if(LOGME)fprintf(flog,"getaddrinfo() patching ipv6\n");
            memset(&in6->sin6_addr, 0, r->ai_addrlen);
            memcpy(&in6->sin6_addr, &addr, sizeof(addr));
        }
        break;
        }
    }

if(LOGME)fprintf(flog,"getaddrinfo() returns %d\n", rc);
    return rc;
}


extern "C" int inet_pton(int af, const char *src, void *dst) {
//TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"inet_pton(%d, %s)\n",af,src);

    int addrlen = 0;
    uint32_t addr = 0;

    switch (af) {
    case AF_INET: //not implemented yet
        addrlen = sizeof(struct in_addr);
        addr = getAddrOfHost(strcmp(src, "0.0.0.0") != 0 ? src : ~GetEnv("HOSTNAME"));
if(LOGME)fprintf(stderr,"inet_pton(AF_INET, %s) addr %#x\n",strcmp(src, "0.0.0.0") != 0 ? src : ~GetEnv("HOSTNAME"),addr);
    break;
    case AF_INET6: {
        addrlen = sizeof(struct in6_addr);
        addr = getAddrOfHost(strcmp(src, "::") != 0 ? src : ~GetEnv("HOSTNAME"));
if(LOGME)fprintf(stderr,"inet_pton(AF_INET6, %s) addr %#x\n",strcmp(src, "0.0.0.0") != 0 ? src : ~GetEnv("HOSTNAME"),addr);
    }
    break;
    }

    if (addr == 0) {
if(LOGME)fprintf(stderr,"inet_pton() addr NOT found\n");
        return _inet_pton(af, src, dst);
    }
if(LOGME)fprintf(stderr,"inet_pton() addr %#x found\n",addr);

    addr = htonl(addr);
    memset(dst, 0, addrlen);
    memcpy(dst, &addr, 4);
for(int jj=0; jj<16;++jj)if(LOGME)fprintf(flog,".%d",((char*)dst)[jj]);if(LOGME)fprintf(flog,"\n%#x\n",addr);
    return 0;
}


#include <errno.h>

extern "C" int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
TMutex mx; TGuard<TMutex> lock(mx);

if(LOGME)fprintf(flog,"connect %d\n",addrlen);

    switch (addrlen) {
    case sizeof(struct sockaddr_in): {
        struct sockaddr_in *in = (struct sockaddr_in *) addr;
if(LOGME)fprintf(flog,"ipv4 sin_port=%d\n",in->sin_port);
        uint32_t s_addr = ntohl(in->sin_addr.s_addr);
        if (s_addr >= 0xC0A80101UL && s_addr < 0xC1000000UL) {
if(LOGME)fprintf(flog,"fake-ipv4\n");
            uint32_t family = in->sin_family;
            uint32_t fakeaddr = in->sin_addr.s_addr;
            uint16_t fakeport = in->sin_port;
            in->sin_port = getBoundPort(family, fakeaddr, fakeport);
            if (in->sin_port == 0) {
if(LOGME)fprintf(flog,"ipv4 REFUSED\n");
            errno=ECONNREFUSED; return -1;}
            //_inet_pton(AF_INET, "0.0.0.0", (void*)&in->sin_addr.s_addr);
            in->sin_addr.s_addr = INADDR_LOOPBACK;
if(LOGME)fprintf(flog,"connect(): fake-ipv4 %#x:%d :%d\n",fakeaddr,fakeport,in->sin_port);
        }
    }
    break;

    case sizeof(struct sockaddr_in6): {
        struct sockaddr_in6 *in6 = (struct sockaddr_in6 *) addr;
        uint32_t *sin6_addr = (uint32_t*) in6->sin6_addr.s6_addr;
        uint32_t s_addr = ntohl(*sin6_addr);
if(LOGME)fprintf(flog,"connect(): ipv6 %d.%d %#x port=%d\n",in6->sin6_addr.s6_addr[0],in6->sin6_addr.s6_addr[15], *sin6_addr, ntohs(in6->sin6_port));
for(int jj=0; jj<16;++jj)if(LOGME)fprintf(flog,".%d",in6->sin6_addr.s6_addr[jj]);if(LOGME)fprintf(flog,"\n");
        if (s_addr >= 0xC0A80101UL && s_addr < 0xC1000000UL) {
            uint32_t fakeaddr = *sin6_addr;
            uint16_t fakeport = in6->sin6_port;
            uint32_t family = in6->sin6_family;
            in6->sin6_port = getBoundPort(family, fakeaddr, fakeport);
            if (in6->sin6_port == 0) {
if(LOGME)fprintf(flog,"ipv6 REFUSED\n");
            errno=ECONNREFUSED; return -1;}
            //_inet_pton(AF_INET6, "::", (void*)&in6->sin6_addr.s6_addr);
            in6->sin6_addr = in6addr_loopback;
if(LOGME)fprintf(flog,"connect(): fake-ipv6 %#x:%d :%d\n",fakeaddr,fakeport,in6->sin6_port);
        }
    }
    break;
    }

    int rc = _connect(sockfd, addr, addrlen);
/*
    if (rc == -1 && errno == EINPROGRESS) {
        fd_set set;
        FD_ZERO (&set);
        FD_SET (sockfd, &set);

        struct timeval tv;
        tv.tv_sec = 60;
        tv.tv_usec = 0;
        rc = select(FD_SETSIZE, NULL, &set, NULL, &tv);
        if (rc == -1) {
            fprintf(flog,"connect-select rc=%d  %d\n",rc,errno);
        }
        else
        if (rc == 0) {
            fprintf(flog,"nothing happened!\n");
        }
        else
        if (FD_ISSET(sockfd, &set)) {
            //connected
            fprintf(flog,"connected!\n");
            rc = 0;
        }
    }
*/
if(LOGME)fprintf(flog,"connect rc=%d  %d\n",rc,errno);
    return rc;
}


extern "C" int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
TMutex mx; TGuard<TMutex> lock(mx);
if(LOGME)fprintf(flog,"bind %d\n",addrlen);

    int rc = -1;

    switch (addrlen) {
    case sizeof(struct sockaddr_in): {
        struct sockaddr_in *in = (struct sockaddr_in *) addr;
if(LOGME)fprintf(flog,"ipv4 sin_port=%d\n",in->sin_port);
        uint32_t s_addr = ntohl(in->sin_addr.s_addr);
        if (s_addr >= 0xC0A80101UL && s_addr < 0xC1000000UL) {
if(LOGME)fprintf(flog,"oops! fake ip4\n");
            int family = in->sin_family;
            uint32_t fakeaddr = in->sin_addr.s_addr;
            uint16_t fakeport = in->sin_port;
            if (getBoundPort(family, fakeaddr, fakeport)) {
if(LOGME)fprintf(flog,"fake ip4 already bound\n");
                return -1;
            }

            in->sin_addr.s_addr = INADDR_ANY;
            for (uint16_t port = ntohs(in->sin_port); port <= 32768; ++port) {
                rc = _bind(sockfd, addr, addrlen);
if(LOGME)fprintf(flog,"bind4 rc=%d\n",rc);
                if (rc == 0) {
                    putBoundPort(in->sin_family, fakeaddr, fakeport, in->sin_port);
                    break;
                }
                in->sin_port = htons(port);
            }
            return rc;
        }
    }
    break;
    case sizeof(struct sockaddr_in6): {
        struct sockaddr_in6 *in6 = (struct sockaddr_in6 *) addr;
        uint32_t *sin6_addr = (uint32_t*) in6->sin6_addr.s6_addr;
        uint32_t s_addr = ntohl(*sin6_addr);
if(LOGME)fprintf(flog,"ipv6 %d.%d %#X port=%d\n",in6->sin6_addr.s6_addr[0],in6->sin6_addr.s6_addr[15], s_addr, ntohs(in6->sin6_port));
for(int jj=0; jj<16;++jj)if(LOGME)fprintf(flog,".%d",in6->sin6_addr.s6_addr[jj]);if(LOGME)fprintf(flog,"\n");
        if (s_addr >= 0xC0A80101UL && s_addr < 0xC1000000UL) {
            if(LOGME)fprintf(flog,"oops! fake ip6\n");
            uint32_t fakeaddr = *sin6_addr;
            uint16_t fakeport = in6->sin6_port;
            uint16_t family = in6->sin6_family;
            if (getBoundPort(family, fakeaddr, fakeport)) {
if(LOGME)fprintf(flog,"fake ip6 already bound\n");
                return -1;
            }

            in6->sin6_addr = in6addr_any;
            for (uint16_t port = ntohs(in6->sin6_port); port <= 32768; ++port) {
                rc = _bind(sockfd, addr, addrlen);
if(LOGME)fprintf(flog,"bind6 rc=%d trying port %d\n",rc, port);
                if (rc == 0) {
                    putBoundPort(in6->sin6_family, fakeaddr, fakeport, in6->sin6_port);
                    break;
                }
                in6->sin6_port = htons(port);
            }
            return rc;
        }
    }
    break;
    }

    rc = _bind(sockfd, addr, addrlen);
if(LOGME)fprintf(flog,"bind rc=%d\n",rc);
    return rc;
}

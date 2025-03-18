#include "vbox_net_wrapper.h"

#include "vbox_common.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/utsname.h>

#include <util/stream/file.h>
#include <util/system/env.h>

#include <dlfcn.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

// helpers
uint32_t ip4(uint8_t ip6[16]) {
    return *(uint32_t*) (ip6 + 2);
}

void toIp6(uint32_t ip4, uint8_t ip6[16]) {
    memset(ip6, 0, sizeof(in6_addr));
    *(uint16_t*) ip6 = 0x0220;
    *(uint32_t*) (ip6 + 2) = ip4;
    *(ip6 + sizeof(in6_addr) - 1) = 1;
}

int findFakeAddrN(const char * name) {
    TUnbufferedFileInput in(GetEnv("VBOX_HOSTS"));
    TString vhost;
    in_addr addr;
    inet_aton("192.168.1.1", &addr);

    uint32_t s_addr = ntohl(addr.s_addr);

    while(in.ReadLine(vhost)) {
        if (vhost.equal(name))
            return htonl(s_addr);
        s_addr += 256;
    }
    return 0;
}

uint16_t findBoundPort(int family, uint32_t addr, uint16_t port) {
    char fname[PATH_MAX];
    sprintf(fname, "%s/vboxhosts.map", GetEnv("VBOX_ROOT").data());

    FILE *in = fopen(fname, "r");
    if (!in)
        return 0;

    flock(fileno(in), LOCK_SH);
    uint16_t dport = 0;

    while (!feof(in)) {
        int sfamily = 0;
        fread(&sfamily, 1, 4, in);

        uint32_t saddr = 0;
        fread(&saddr, 1, 4, in);

        uint16_t sport = 0;
        fread(&sport, 1, 2, in);

        dport = 0;
        fread(&dport, 1, 2, in);

        logme(net, "findBoundPort(): saddr=%#x sport=%d dport=%d", ntohl(saddr), ntohs(sport), ntohs(dport));
        if (family == sfamily && addr == saddr && port == sport) {
            logme(net, "findBoundPort(): bingo!");
            break;
        }
    }

    flock(fileno(in), LOCK_UN);
    fclose(in);
    return dport;
}

uint16_t findFakePort(int family, uint32_t vaddr, uint16_t rport) {
    char fname[PATH_MAX];
    sprintf(fname, "%s/vboxhosts.map", GetEnv("VBOX_ROOT").data());

    FILE *in = fopen(fname, "r");
    if (!in)
        return 0;

    flock(fileno(in), LOCK_SH);
    uint16_t sport = 0;

    while (!feof(in)) {
        int sfamily = 0;
        fread(&sfamily, 1, 4, in);

        uint32_t saddr = 0;
        fread(&saddr, 1, 4, in);

        sport = 0;
        fread(&sport, 1, 2, in);

        uint16_t dport = 0;
        fread(&dport, 1, 2, in);

        logme(net, "findBoundPort(): saddr=%#x sport=%d dport=%d", ntohl(saddr), ntohs(sport), ntohs(dport));
        if (family == sfamily && vaddr == saddr && rport == dport) {
            logme(net, "findFakePort(): bingo!");
            break;
        }
    }

    flock(fileno(in), LOCK_UN);
    fclose(in);
    return sport;
}

void unmapBoundPort(int family, uint32_t addr, uint16_t port) {
    char fname[PATH_MAX];
    sprintf(fname, "%s/vboxhosts.map", GetEnv("VBOX_ROOT").data());

    FILE *in = fopen(fname, "r+");
    if (!in)
        return;

    flock(fileno(in), LOCK_EX);
    while (!feof(in)) {
        size_t pos = ftell(in);

        int sfamily = 0;
        fread(&sfamily, 1, 4, in);

        uint32_t saddr = 0;
        fread(&saddr, 1, 4, in);

        uint16_t sport = 0;
        fread(&sport, 1, 2, in);

        uint16_t dport = 0;
        fread(&dport, 1, 2, in);

        if (family == sfamily && addr == saddr && port == sport) {
            logme(net, "unmapBoundPort(): bingo!");
            fseek(in, 1, pos);
            int family = -1;
            fwrite(&family, 1, 4, in);
            break;
        }
    }

    flock(fileno(in), LOCK_UN);
    fclose(in);
}

void mapBoundPort(int family, uint32_t saddr, uint16_t sport, uint16_t dport) {
    uint16_t port = findBoundPort(family, saddr, sport);
    if (port > 0)
        return;

    char fname[PATH_MAX];
    sprintf(fname, "%s/vboxhosts.map", GetEnv("VBOX_ROOT").data());

    FILE *out = fopen(fname, "a+");
    if (!out)
        abort();
    flock(fileno(out), LOCK_EX);
    fwrite(&family, 1, 4, out);
    fwrite(&saddr, 1, 4, out);
    fwrite(&sport, 1, 2, out);
    fwrite(&dport, 1, 2, out);

    flock(fileno(out), LOCK_UN);
    fclose(out);
}


typedef int libc_getaddrinfo_fn(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
static libc_getaddrinfo_fn *libc_getaddrinfo = nullptr;

typedef int libc_gethostname_fn(const char *name, size_t len);
static libc_gethostname_fn *libc_gethostname = nullptr;

typedef struct hostent *libc_gethostbyname_fn(const char *name);
static libc_gethostbyname_fn *libc_gethostbyname = nullptr;

#ifdef FREEBSD
int __xuname_wrapper(int len, void *buf) {
    logme(net, "__xuname(%d, %p)", len, buf);

    typedef int libc___xuname_fn(int len, void *buf);
    static libc___xuname_fn *libc___xuname = 0;
    if (!libc___xuname) libc___xuname = (libc___xuname_fn*) dlsym(RTLD_NEXT, "__xuname");

    int rc = libc___xuname(len, buf);
    strcpy(((struct utsname *)buf)->nodename, ~GetEnv("HOSTNAME"));
    return rc;
}
#else
int uname_wrapper(struct utsname *buf) {
    logme(net, "uname(%p)", buf);

    typedef int libc_uname_fn(struct utsname *buf);
    static libc_uname_fn *libc_uname = nullptr;
    if (!libc_uname) libc_uname = (libc_uname_fn*) dlsym(RTLD_NEXT, "uname");

    int rc = libc_uname(buf);
    strcpy(buf->nodename, GetEnv("HOSTNAME").data());
    return rc;
}
#endif

int gethostname_wrapper(char *name, size_t len) {
    logme(net, "gethostname()");
    if (!libc_gethostname) libc_gethostname = (libc_gethostname_fn*) dlsym(RTLD_NEXT, "gethostname");

    strncpy(name, GetEnv("HOSTNAME").data(), len);
    return 0;
}

struct hostent *gethostbyname_wrapper(const char *name) {
    logme(net, "gethostbyname(%s)", name);
    if (!libc_gethostbyname) libc_gethostbyname = (libc_gethostbyname_fn*) dlsym(RTLD_NEXT, "gethostbyname");

    static struct hostent *host = libc_gethostbyname(name);

    uint32_t addr = findFakeAddrN(name);
    if (addr) {
//XXX:        memcpy(host->h_addr, &addr, host->h_length);
    }

    return host;
}

int getaddrinfo_wrapper(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    logme(net, "getaddrinfo(%s, %s)", node, service);
    if (!libc_getaddrinfo) libc_getaddrinfo = (libc_getaddrinfo_fn*) dlsym(RTLD_NEXT, "getaddrinfo");
    if (!libc_gethostname) libc_gethostname = (libc_gethostname_fn*) dlsym(RTLD_NEXT, "gethostname");

    //HACK: worker binds only bo ipv6, but master tries to connect to worker on ipv4 (if worker's ipv4 comes first)
    ((struct addrinfo*)hints)->ai_family = AF_INET6;

    if (node && !strcmp(node, "0.0.0.0")) {
        node = nullptr;
    }
    uint32_t addr = findFakeAddrN(node ? node : GetEnv("HOSTNAME").data());
    if (addr == 0) {
        logme(net, "getaddrinfo(%s, %s) or addr 0", node, service);
        return libc_getaddrinfo(node, service, hints, res);
    }

    char thishost[4096] = {0};
    libc_gethostname(thishost, sizeof(thishost));

    int rc = libc_getaddrinfo(thishost, service, hints, res);
    logme(net, "getaddrinfo(%s, %s): fakeaddr=%#x, rc=%d", node, service, ntohl(addr), rc);
    if (rc != 0) return rc;

    for (struct addrinfo *r = *res; r != nullptr; r = r->ai_next) {
        switch (r->ai_family) {
            case AF_INET: {
                struct sockaddr_in *in4 = (struct sockaddr_in *) r->ai_addr;
                in4->sin_addr.s_addr = addr;
                break;
            }
            case AF_INET6: {
                struct sockaddr_in6 *in6 = (struct sockaddr_in6 *) r->ai_addr;
                memset(&in6->sin6_addr, 0, sizeof(in6->sin6_addr));
                toIp6(addr, in6->sin6_addr.s6_addr);
                break;
            }
        }
    }

    return rc;
}

int inet_pton_wrapper(int af, const char *src, void *dst) {
    logme(net, "inet_pton(%d, %s)", af, src);

    typedef int libc_inet_pton_fn(int af, const char *src, void *dst);
    static libc_inet_pton_fn *libc_inet_pton = nullptr;
    if (!libc_inet_pton) libc_inet_pton = (libc_inet_pton_fn*) dlsym(RTLD_NEXT, "inet_pton");

    int addrlen = 0;
    uint32_t addr = 0;

    switch (af) {
        case AF_INET: {
            addrlen = sizeof(struct in_addr);
            addr = findFakeAddrN(strcmp(src, "0.0.0.0") != 0 ? src : GetEnv("HOSTNAME").data());
            logme(net, "inet_pton(AF_INET, %s) fakeaddr=%#x", strcmp(src, "0.0.0.0") != 0 ? src : GetEnv("HOSTNAME").data(), addr);
            memcpy(dst, &addr, 4);
            break;
        }
        case AF_INET6: {
            addrlen = sizeof(struct in6_addr);
            addr = findFakeAddrN(strcmp(src, "::") != 0 ? src : GetEnv("HOSTNAME").data());
            logme(net, "inet_pton(AF_INET6, %s) fakeaddr=%#x", strcmp(src, "0.0.0.0") != 0 ? src : GetEnv("HOSTNAME").data(), addr);
            toIp6(addr, (uint8_t*) dst);
            break;
        }
    }

    if (addr == 0) {
        return libc_inet_pton(af, src, dst);
    }

    return 0;
}

int connect_wrapper(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    logme(net, "connect(%d, addrlen=%d)", sockfd, addrlen);

    typedef int libc_connect_fn(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    static libc_connect_fn *libc_connect = nullptr;
    if (!libc_connect) libc_connect = (libc_connect_fn*) dlsym(RTLD_NEXT, "connect");

    in_addr from, to;
    inet_aton("192.168.1.1", &from);
    inet_aton("192.168.255.1", &to);

    switch (addrlen) {
        case sizeof(struct sockaddr_in): {
            struct sockaddr_in in(*(struct sockaddr_in *) addr);
            uint32_t s_addr = in.sin_addr.s_addr;

            if (s_addr >= from.s_addr && s_addr <= to.s_addr) {
                int family = in.sin_family;
                uint32_t vaddr = in.sin_addr.s_addr;
                uint16_t vport = in.sin_port;
                in.sin_port = findBoundPort(family, vaddr, vport);
                if (in.sin_port == 0) {
                    errno = ECONNREFUSED;
                    return -1;
                }

                in.sin_addr.s_addr = INADDR_LOOPBACK;
                logme(net, "connect4(%d): addr=%#x port=%d", sockfd, ntohl(vaddr), ntohs(vport));
            }
            return libc_connect(sockfd, (struct sockaddr *) &in, addrlen);
        }

        case sizeof(struct sockaddr_in6): {
            struct sockaddr_in6 in6(*(struct sockaddr_in6 *) addr);
            uint32_t s_addr = ip4(in6.sin6_addr.s6_addr);

            if (s_addr >= from.s_addr && s_addr <= to.s_addr) {
                int family = in6.sin6_family;
                uint32_t vaddr = s_addr;
                uint16_t vport = in6.sin6_port;
                in6.sin6_port = findBoundPort(family, vaddr, vport);
                if (in6.sin6_port == 0) {
                    errno = ECONNREFUSED;
                    return -1;
                }

                in6.sin6_addr = in6addr_loopback;
                logme(net, "connect6(%d): addr=%#x port=%d", sockfd, ntohl(vaddr), ntohs(vport));
            }
            return libc_connect(sockfd, (struct sockaddr *) &in6, addrlen);
        }
    }

    int rc = libc_connect(sockfd, addr, addrlen);
    logme(net, "connect(%d): rc=%d", sockfd, rc);
    return rc;
}

int bind_wrapper(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    logme(net, "bind(%d, addrlen=%d)", sockfd, addrlen);

    typedef int libc_bind_fn(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    static libc_bind_fn *libc_bind = nullptr;
    if (!libc_bind) libc_bind = (libc_bind_fn*) dlsym(RTLD_NEXT, "bind");

    int rc = -1;
    in_addr from, to;
    inet_aton("192.168.1.1", &from);
    inet_aton("192.168.255.1", &to);

    switch (addrlen) {
        case sizeof(struct sockaddr_in): {
            struct sockaddr_in in(*(struct sockaddr_in *) addr);
            uint32_t s_addr = in.sin_addr.s_addr;

            logme(net, "bind4(): s_addr=%#x from=%#x to=%#x", ntohl(s_addr), ntohl(from.s_addr), ntohl(to.s_addr));
            if (s_addr >= from.s_addr && s_addr <= to.s_addr) {
                int family = in.sin_family;
                uint32_t vaddr = in.sin_addr.s_addr;
                uint16_t vport = in.sin_port;

                uint16_t bound_port = findBoundPort(family, vaddr, vport);
                if (bound_port > 0) {
                    // try to bind, maybe process was killed
                    in.sin_addr.s_addr = INADDR_ANY;
                    in.sin_port = bound_port;
                    int rc = libc_bind(sockfd, (struct sockaddr *) &in, addrlen);
                    logme(net, "tried to rebind4: rc=%d", rc);
                    return rc;
                }

                in.sin_addr.s_addr = INADDR_ANY;
                in.sin_port = vport;
                for (uint16_t port = ntohs(vport); port <= 32768; ++port) {
                    rc = libc_bind(sockfd, (struct sockaddr *) &in, addrlen);
                    logme(net, "bind(AF_INET, %#x : %d): redir_port=%d rc=%d", ntohl(vaddr), ntohs(vport), port, rc);
                    if (rc == 0) {
                        mapBoundPort(family, vaddr, vport, in.sin_port);
                        break;
                    }
                    in.sin_port = htons(port);
                }
                return rc;
            }
            break;
        }

        case sizeof(struct sockaddr_in6): {
            struct sockaddr_in6 in6(*(struct sockaddr_in6 *) addr);
            uint32_t s_addr = ip4(in6.sin6_addr.s6_addr);

            logme(net, "bind6(): s_addr=%#x from=%#x to=%#x", ntohl(s_addr), ntohl(from.s_addr), ntohl(to.s_addr));
            if (s_addr >= from.s_addr && s_addr <= to.s_addr) {
                int family = in6.sin6_family;
                uint32_t vaddr = s_addr;
                uint16_t vport = in6.sin6_port;

                uint16_t bound_port = findBoundPort(family, vaddr, vport);
                if (bound_port > 0) {
                    // try to bind, maybe process was killed
                    in6.sin6_addr = in6addr_any;
                    in6.sin6_port = bound_port;
                    int rc = libc_bind(sockfd, (struct sockaddr *) &in6, addrlen);
                    logme(net, "tried to rebind6: rc=%d", rc);
                    return rc;
                }

                in6.sin6_addr = in6addr_any;
                in6.sin6_port = vport;
                for (uint16_t port = ntohs(vport); port <= 32768; ++port) {
                    rc = libc_bind(sockfd, (struct sockaddr *) &in6, addrlen);
                    logme(net, "bind(AF_INET6, %#x : %d): redir_port=%d rc=%d", ntohl(vaddr), ntohs(vport), port, rc);
                    if (rc == 0) {
                        mapBoundPort(family, vaddr, vport, in6.sin6_port);
                        break;
                    }
                    in6.sin6_port = htons(port);
                }
                return rc;
            }
            break;
        }
    }

    rc = libc_bind(sockfd, addr, addrlen);
    logme(net, "bind(%d): rc=%d", sockfd, rc);
    return rc;
}


int close_wrapper(int fd) {
    logme(net, "close(%d)", fd);

    typedef int libc_close_fn(int fd);
    static libc_close_fn *libc_close = nullptr;
    if (!libc_close) libc_close = (libc_close_fn*) dlsym(RTLD_NEXT, "close");

    return libc_close(fd);
}


int getsockname_wrapper(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    logme(net, "getsockname(%d)", sockfd);

    typedef int libc_getsockname_fn(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
    static libc_getsockname_fn *libc_getsockname = nullptr;
    if (!libc_getsockname) libc_getsockname = (libc_getsockname_fn*) dlsym(RTLD_NEXT, "getsockname");

    int rc = libc_getsockname(sockfd, addr, addrlen);
    return rc;
    if (rc != 0) return rc;

    switch (*addrlen) {
        case sizeof(struct sockaddr_in): {
            struct sockaddr_in *in = (struct sockaddr_in *) addr;

            if (in->sin_addr.s_addr == INADDR_LOOPBACK) {
                int family = in->sin_family;
                uint32_t vaddr = findFakeAddrN(GetEnv("HOSTNAME").data());
                uint16_t rport = in->sin_port;
                uint16_t vport = findFakePort(family, vaddr, rport);

                if (vport) {
                    in->sin_addr.s_addr = vaddr;
                    in->sin_port = vport;
                }
            }
            break;
        }

        case sizeof(struct sockaddr_in6): {
            struct sockaddr_in6 *in6 = (struct sockaddr_in6 *) addr;
            if (memcmp(&in6->sin6_addr.s6_addr, &in6addr_loopback.s6_addr, sizeof in6->sin6_addr) == 0) {
                int family = in6->sin6_family;
                uint32_t vaddr = findFakeAddrN(GetEnv("HOSTNAME").data());
                uint16_t rport = in6->sin6_port;
                uint16_t vport = findFakePort(family, vaddr, rport);

                if (vport) {
                    toIp6(vaddr, in6->sin6_addr.s6_addr);
                    in6->sin6_port = vport;
                }
            }
            break;
        }
    }

    return 0;
}

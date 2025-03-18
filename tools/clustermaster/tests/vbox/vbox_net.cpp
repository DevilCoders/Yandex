#include "vbox_net_wrapper.h"

#ifdef FREEBSD
extern "C" int __xuname(int len, void *buf);
    return __xuname_wrapper(len, buf);
#else
extern "C" int uname(struct utsname *buf) {
    return uname_wrapper(buf);
}
#endif

extern "C" int gethostname(char *name, size_t len) {
    return gethostname_wrapper(name, len);
}

extern "C" struct hostent *gethostbyname(const char *name) {
    return gethostbyname_wrapper(name);
}

extern "C" int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    return getaddrinfo_wrapper(node, service, hints, res);
}

extern "C" int inet_pton(int af, const char *src, void *dst) {
    return inet_pton_wrapper(af, src, dst);
}

extern "C" int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_wrapper(sockfd, addr, addrlen);
}

extern "C" int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return bind_wrapper(sockfd, addr, addrlen);
}
/*
extern "C" int close(int fd) {
    return close_wrapper(fd);
}

extern "C" int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    return getsockname_wrapper(sockfd, addr, addrlen);
}
*/

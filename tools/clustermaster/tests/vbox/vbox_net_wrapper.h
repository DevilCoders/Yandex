#pragma once

#include <sys/types.h>

extern "C" {

#ifdef FREEBSD
int __xuname_wrapper(int len, void *buf);
#else
struct utsname;
int uname_wrapper(struct utsname *buf);
#endif

int gethostname_wrapper(char *name, size_t len);

struct hostent;
struct hostent *gethostbyname_wrapper(const char *name);

struct addrinfo;
int getaddrinfo_wrapper(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);

int inet_pton_wrapper(int af, const char *src, void *dst);

struct sockaddr;
typedef unsigned int socklen_t;

int connect_wrapper(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int bind_wrapper(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

int close_wrapper(int fd);

int getsockname_wrapper(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

}

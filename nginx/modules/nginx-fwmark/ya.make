LIBRARY()

OWNER(
    toshik
    g:contrib
)

BUILD_ONLY_IF(LINUX)

NO_UTIL()

PEERDIR(
    contrib/nginx/core/src/http
)

ADDINCL(
    contrib/nginx/core/objs
)

SRCS(
    ngx_http_fwmark_module.c
)

END()

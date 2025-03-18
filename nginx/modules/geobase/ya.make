LIBRARY()

OWNER(
    toshik
    g:contrib
)

BUILD_ONLY_IF(LINUX)

NO_UTIL()

CFLAGS(-Wno-unused-parameter)

PEERDIR(
    contrib/nginx/core/src/http
    geobase/library
)

ADDINCL(
    contrib/nginx/core/objs
)

SRCS(
    ngx_http_geobase_module.c
    ngx_http_geobase_module.cpp
)

END()

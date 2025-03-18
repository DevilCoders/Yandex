LIBRARY()

OWNER(
    ashaposhnikov
    g:contrib
)

BUILD_ONLY_IF(LINUX)

NO_UTIL()

CFLAGS(-Wno-unused-parameter)

PEERDIR(
    contrib/nginx/core/src/http
)

SRCS(
    src/ngx_http_force_gunzip_module.c
)

END()

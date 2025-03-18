LIBRARY()

OWNER(
    ezaitov
    g:contrib
    g:antirobot
)

BUILD_ONLY_IF(LINUX)

NO_UTIL()

CFLAGS(-Wno-unused-parameter)

PEERDIR(
    contrib/nginx/core/src/http
)

ADDINCL(
    contrib/nginx/core/objs
)

SRCS(
    src/ngx_http_yandex_antirobot_module.c
)

END()

LIBRARY()

OWNER(
    g:strm-admin
    nyoroon
)

BUILD_ONLY_IF(LINUX)

NO_UTIL()

CFLAGS(-Wno-unused-parameter)

PEERDIR(
    contrib/nginx/core/src/http
    yweb/webdaemons/icookiedaemon/icookie_lib
)

SRCS(
    ngx_http_yandex_icookie_module.cpp
)

END()

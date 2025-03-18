LIBRARY()

OWNER(
    g:strm-admin
    goodfella
)

BUILD_ONLY_IF(LINUX)

CFLAGS(-Wno-unused-parameter)

PEERDIR(
    contrib/nginx/core/src/http
    contrib/libs/util-linux
    kernel/p0f/bpf
    kernel/p0f/load
    kernel/p0f/format
)

SRCS(
    ngx_http_p0f_module.c
    p0f_bridge.cpp
)

END()

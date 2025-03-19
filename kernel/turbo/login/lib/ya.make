RECURSE(
    crypto
    py
)

LIBRARY()

OWNER(
    g:turbo
    desertfury
)

PEERDIR(
    kernel/turbo/canonizer
    library/cpp/config
    library/cpp/eventlog
    library/cpp/json
    library/cpp/string_utils/url
    quality/functionality/turbo/runtime/common
    quality/functionality/turbo/urls_lib/cpp/lib
    apphost/api/service/cpp
    web/src_setup/lib/setup/common/ctxresponse
)

SRCS(
    turbo_login.cpp
    turbo_login.h
)

END()

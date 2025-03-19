LIBRARY()

OWNER(iddqd)

WERROR()

PEERDIR(
    kernel/daemon/config
    library/cpp/http/misc
    library/cpp/http/server
    library/cpp/logger/global
    kernel/common_proxy/common
)

SRCS(
    GLOBAL registrar.cpp
    http.cpp
)

END()

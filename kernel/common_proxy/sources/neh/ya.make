LIBRARY()

OWNER(iddqd)

WERROR()

PEERDIR(
    kernel/daemon/config
    library/cpp/neh
    library/cpp/logger/global
    kernel/common_proxy/common
)

SRCS(
    GLOBAL registrar.cpp
    neh.cpp
)

END()

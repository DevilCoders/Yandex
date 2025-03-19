LIBRARY()

OWNER(ivanmorozov)

PEERDIR(
    kernel/daemon/config
    library/cpp/cgiparam
    library/cpp/logger/global
    library/cpp/object_factory
)

SRCS(
    module.cpp
)

END()

LIBRARY()

OWNER(g:cs_dev)

SRCS(
    abstract.cpp
)

PEERDIR(
    library/cpp/yconf
    library/cpp/object_factory
    kernel/common_server/common
    kernel/common_server/library/logging
    kernel/common_server/abstract
    kernel/daemon/config
)

END()

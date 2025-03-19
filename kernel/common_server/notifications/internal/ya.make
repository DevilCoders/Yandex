LIBRARY()

OWNER(g:cs_dev)

SRCS(
    GLOBAL internal.cpp
)

PEERDIR(
    library/cpp/yconf
    kernel/common_server/util/network
    kernel/common_server/notifications/abstract
)

END()

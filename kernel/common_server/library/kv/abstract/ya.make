LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/library/interfaces
    kernel/common_server/library/logging
    kernel/common_server/util
)

SRCS(
    storage.cpp
    config.cpp
)

END()

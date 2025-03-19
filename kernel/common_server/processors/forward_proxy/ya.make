LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/api/history
    kernel/common_server/processors/common
    kernel/common_server/library/storage
)

SRCS(
    GLOBAL handler.cpp
)

END()

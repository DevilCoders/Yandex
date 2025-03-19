LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/api/history
    kernel/common_server/util
)

SRCS(
    GLOBAL storage.cpp
)

END()

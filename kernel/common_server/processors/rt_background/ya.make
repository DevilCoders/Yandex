LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/processors/common
    kernel/common_server/rt_background
)

SRCS(
    GLOBAL handler.cpp
)

END()

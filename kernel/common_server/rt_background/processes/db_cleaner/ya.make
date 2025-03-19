LIBRARY()

OWNER(g:cs_dev)

SRCS(
    GLOBAL process.cpp
)

PEERDIR(
    kernel/common_server/rt_background
    kernel/common_server/rt_background/processes/common
)

END()

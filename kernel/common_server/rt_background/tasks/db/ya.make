LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/rt_background/tasks/abstract
    kernel/common_server/rt_background/tasks/db/proto
)

SRCS(
    GLOBAL manager.cpp
    object.cpp
)

END()

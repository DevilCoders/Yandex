LIBRARY()

OWNER(g:cs_dev)

SRCS(
    database.cpp
    state.cpp
    yt.cpp
)

PEERDIR(
    kernel/common_server/rt_background
    kernel/common_server/proto
    kernel/common_server/library/unistat
    kernel/common_server/util
    kernel/common_server/library/yt/node

    mapreduce/yt/interface
)

END()

LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    kernel/common_server/api/history
    kernel/common_server/util
    library/cpp/yson/node
    mapreduce/yt/client
)

SRCS(
    GLOBAL context.cpp
    GLOBAL content.cpp
)

END()

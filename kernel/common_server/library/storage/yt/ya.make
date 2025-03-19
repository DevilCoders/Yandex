LIBRARY()

OWNER(g:cs_dev)

PEERDIR(
    mapreduce/yt/interface
    mapreduce/yt/client
    kernel/common_server/library/storage
)

SRCS(
    GLOBAL storage.cpp
    GLOBAL config.cpp
)

END()

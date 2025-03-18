LIBRARY()

OWNER(g:aapi)

PEERDIR(
    aapi/lib/trace
    mapreduce/yt/interface
    mapreduce/yt/client
    library/cpp/blockcodecs
    library/cpp/threading/future
    library/cpp/threading/blocking_queue
    contrib/libs/grpc
)

SRCS(
    async_lookups.cpp
    async_lookups2.cpp
)

END()

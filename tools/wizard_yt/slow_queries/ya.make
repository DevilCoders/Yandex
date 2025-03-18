PROGRAM()

OWNER(
    g:wizard
)

PEERDIR(
    library/cpp/eventlog
    library/cpp/getopt
    ysite/yandex/reqdata
    mapreduce/yt/client
    mapreduce/yt/interface
    mapreduce/yt/common
)

ALLOCATOR(LF)

SRCS(
    slow_queries.cpp
)

END()

LIBRARY()

OWNER(g:cs_dev)

SRCS(
    GLOBAL process.cpp
    GLOBAL yt_dumper.cpp
    GLOBAL memory_dumper.cpp
    GLOBAL persistent_queue_dumper.cpp
    meta_parser.cpp
    dumper.cpp
    table_field_viewer.cpp
)

PEERDIR(
    kernel/common_server/rt_background
    kernel/common_server/rt_background/processes/common
    kernel/common_server/library/yt/common
    kernel/common_server/library/persistent_queue/abstract

    mapreduce/yt/client
)

END()

RECURSE_FOR_TESTS(
    ut
)
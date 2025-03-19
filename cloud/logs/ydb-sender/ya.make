OWNER(lvovich)

PROGRAM()

#ALLOCATOR(LF_YT)
ALLOCATOR(LF_DBG)

SRCS(
    log_converter.cpp
    pq_reader.cpp
    ydb_writer.cpp
    monitoring.cpp
    sender.cpp
)

PEERDIR(
    kikimr/persqueue/sdk/deprecated/cpp/v2
    library/cpp/getopt
    library/cpp/lfalloc/alloc_profiler
    library/cpp/logger
    library/cpp/monlib/encode
    library/cpp/monlib/metrics
    library/cpp/monlib/service
    library/cpp/monlib/service/pages
    library/cpp/yson/json
    logfeller/lib/log_parser
    ydb/public/sdk/cpp/client/ydb_driver
    ydb/public/sdk/cpp/client/ydb_table
    ydb/public/sdk/cpp/client/ydb_value
)

END()

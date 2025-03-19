UNITTEST_FOR(cloud/logs/ydb-sender)

OWNER(lvovich)

SRCS(
    log_converter_ut.cpp
    log_converter.cpp
)

PEERDIR(
    kikimr/persqueue/sdk/deprecated/cpp/v2
    library/cpp/getopt
    library/cpp/lfalloc/alloc_profiler
    library/cpp/logger
    logfeller/lib/log_parser
    ydb/public/sdk/cpp/client/ydb_table
)

END()

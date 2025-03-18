PROGRAM()

OWNER(steiner)

PEERDIR(
    library/cpp/getopt
    mapreduce/lib

    quality/functionality/turbo/user_metrics_lib
    quality/logs/parse_lib
    quality/user_search/common
    quality/user_sessions/request_aggregate_lib
)

SRCS(
    main.cpp
)

END()

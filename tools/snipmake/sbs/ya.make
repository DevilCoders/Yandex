OWNER(steiner)

PROGRAM(generate_sbs_tasks)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/charset
    library/cpp/containers/ext_priority_queue
    library/cpp/getopt
    library/cpp/string_utils/quote
    mapreduce/lib
    mapreduce/library/multiset_model
    mapreduce/library/temptable
    quality/deprecated/Misc
    quality/trailer/suggest/fastcgi_lib/http
    quality/user_sessions/request_aggregate_lib
)

END()

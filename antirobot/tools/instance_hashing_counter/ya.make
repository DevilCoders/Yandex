PROGRAM()

OWNER(g:antirobot)

PEERDIR(
    antirobot/daemon_lib
    library/cpp/getopt
    library/cpp/ipv6_address
    library/cpp/yson/node
    mapreduce/yt/interface
    mapreduce/yt/client
    mapreduce/yt/util
    mapreduce/yt/interface/protos
)

SRCS(
    main.cpp
    proto/daemon_log_row.proto
    proto/l7_access_log_row.proto
    proto/mapper_output_row.proto
    proto/reducer_output_row.proto
)

END()

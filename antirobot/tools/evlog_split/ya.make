PROGRAM()

OWNER(g:antirobot)

PEERDIR(
    antirobot/idl
    antirobot/tools/evlogdump/lib
    library/cpp/getopt
    library/cpp/iterator
    library/cpp/protobuf/yql
    mapreduce/yt/client
    mapreduce/yt/library/operation_tracker
    mapreduce/yt/util
    security/ant-secret/snooper/cpp
)

SRCS(
    evlog.proto
    main.cpp
)

END()

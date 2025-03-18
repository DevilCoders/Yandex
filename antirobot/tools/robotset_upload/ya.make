PROGRAM()

OWNER(g:antirobot)

PEERDIR(
    antirobot/daemon_lib
    antirobot/idl
    antirobot/lib
    antirobot/tools/robotset_upload/proto
    library/cpp/getopt
    library/cpp/getoptpb
    mapreduce/yt/interface
    mapreduce/yt/util
    mapreduce/yt/client
)

SRCS(
    main.cpp
)

END()
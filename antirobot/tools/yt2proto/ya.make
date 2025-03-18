PROGRAM()

OWNER(g:antirobot)

PEERDIR(
    antirobot/idl
    antirobot/lib
    library/cpp/getopt
    library/cpp/protobuf/yt
    mapreduce/yt/client
)

SRCS(
    main.cpp
)

END()

PROGRAM()

OWNER(g:antirobot)

PEERDIR(
    mapreduce/yt/interface
    mapreduce/yt/client
    antirobot/idl
    library/cpp/http/io
    library/cpp/getopt
)

SRCS(
    source.cpp
)

END()

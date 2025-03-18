PROGRAM()

ALLOCATOR(GOOGLE)

OWNER(kartynnik)

PEERDIR(
    contrib/libs/protobuf
    library/cpp/getopt
    mapreduce/library/io/streaming
    mapreduce/library/io/stream
    yweb/robot/kiwi/protos
)

SRCS(
    main.cpp
)

END()

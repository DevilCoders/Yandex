PROGRAM(host_dump_formatter)

ALLOCATOR(LF)

OWNER(marvelstas)

PEERDIR(
    contrib/libs/protobuf
    library/cpp/getopt
    library/cpp/http/fetch
    library/cpp/svnversion
    library/cpp/uri
    robot/deprecated/gemini/mr/libmrutils
    yweb/robot/kiwi/protos
)

SRCS(
    main.cpp
    config.cpp
)

END()


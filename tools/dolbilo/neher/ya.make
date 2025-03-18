PROGRAM(d-neher)

ALLOCATOR(LF)

OWNER(
    darkk
    mvel
    g:base
)

PEERDIR(
    library/cpp/dolbilo
    library/cpp/getopt
    library/cpp/http/io
    library/cpp/http/misc
    library/cpp/neh
    library/cpp/streams/factory
    library/cpp/uri
    library/cpp/deprecated/atomic
)

SRCS(
    main.cpp
)

END()

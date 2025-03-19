PROGRAM()

OWNER(
    alex-sh
    druxa
)

SRCS(
    main.cpp
)

PEERDIR(
    kernel/ethos/lib/cross_validation
    kernel/ethos/lib/data
    library/cpp/getopt
    library/cpp/linear_regression
    library/cpp/scheme
    library/cpp/streams/factory
)

ALLOCATOR(LF)

END()

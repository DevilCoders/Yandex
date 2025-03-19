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
    kernel/ethos/lib/logistic_regression
    kernel/ethos/lib/metrics
    kernel/ethos/lib/naive_bayes
    kernel/ethos/lib/util
    library/cpp/getopt
    library/cpp/linear_regression
    library/cpp/scheme
    library/cpp/streams/factory
)

ALLOCATOR(LF)

END()

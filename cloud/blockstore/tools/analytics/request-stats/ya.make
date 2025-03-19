OWNER(g:cloud-nbs)

PROGRAM(blockstore-request-stats)

SRCS(
    main.cpp
)

PEERDIR(
    library/cpp/getopt
    library/cpp/json
    library/cpp/linear_regression
    library/cpp/streams/factory
)

END()

PROGRAM(d-planner)

OWNER(
    g:base
    darkk
    mvel
)

PEERDIR(
    library/cpp/coroutine/engine
    library/cpp/dolbilo/planner
    library/cpp/getopt
    library/cpp/streams/factory
)

SRCS(
    main.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)

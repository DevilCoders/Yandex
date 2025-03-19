PROGRAM(eternal-load)

OWNER(g:cloud-nbs)

SRCS(
    app.cpp
    main.cpp
    options.cpp
    test.cpp
)

PEERDIR(
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/tools/testing/eternal-tests/eternal-load/lib

    library/cpp/getopt
    library/cpp/threading/future
    library/cpp/json
)

END()

RECURSE_FOR_TESTS(
    tests
)

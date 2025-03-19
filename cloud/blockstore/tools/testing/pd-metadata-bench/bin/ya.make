PROGRAM(pd-metadata-bench)

OWNER(g:cloud-nbs)

SRCS(
    app.cpp
    main.cpp
    options.cpp
)

PEERDIR(
    cloud/blockstore/tools/testing/pd-metadata-bench/lib

    library/cpp/getopt
    library/cpp/threading/future
)

END()

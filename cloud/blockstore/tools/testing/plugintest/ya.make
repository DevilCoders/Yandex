PROGRAM(blockstore-plugintest)

OWNER(g:cloud-nbs)

SRCS(
    main.cpp
    options.cpp
    test.cpp
    test.proto
)

PEERDIR(
    cloud/vm/api

    cloud/blockstore/config

    library/cpp/getopt
    library/cpp/protobuf/util
)

END()

PROGRAM(blockstore-write-disk-status)

OWNER(g:cloud-nbs)

GENERATE_ENUM_SERIALIZATION(
    options.h
)

SRCS(
    main.cpp
    options.cpp
)

PEERDIR(
    cloud/blockstore/config
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/public/api/protos

    library/cpp/getopt
    library/cpp/getopt/small

    kikimr/persqueue/sdk/deprecated/cpp/v2
)

END()

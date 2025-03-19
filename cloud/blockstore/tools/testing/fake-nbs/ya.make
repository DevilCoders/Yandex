PROGRAM(blockstore-fake-nbs)

OWNER(g:cloud-nbs)

SRCS(
    main.cpp
)

PEERDIR(
    cloud/blockstore/libs/discovery

    library/cpp/getopt
)

END()

PROGRAM(blockstore-cpu-wait-monitor)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics

    library/cpp/getopt
    library/cpp/sighandler
)

SRCS(
    main.cpp
)

END()
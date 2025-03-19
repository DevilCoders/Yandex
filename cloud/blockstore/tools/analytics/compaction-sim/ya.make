PROGRAM(blockstore-compaction-sim)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics/events
    cloud/blockstore/libs/service
    cloud/blockstore/libs/storage/partition/model

    library/cpp/getopt
    library/cpp/eventlog/dumper
    library/cpp/logger
    library/cpp/sighandler
)

SRCS(
    main.cpp
)

END()

PROGRAM(blockstore-dump-event-log)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/blockstore/libs/diagnostics/events
    cloud/blockstore/tools/analytics/libs/event-log

    library/cpp/eventlog/dumper
)

SRCS(
    main.cpp
)

END()

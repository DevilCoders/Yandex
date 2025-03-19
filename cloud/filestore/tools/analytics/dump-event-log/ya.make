PROGRAM(filestore-dump-event-log)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/filestore/libs/diagnostics/events
    cloud/filestore/tools/analytics/libs/event-log

    library/cpp/eventlog/dumper
)

SRCS(
    main.cpp
)

END()

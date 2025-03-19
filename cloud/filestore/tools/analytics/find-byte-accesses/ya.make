PROGRAM(filestore-find-byte-accesses)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/filestore/libs/diagnostics/events
    cloud/filestore/libs/storage/tablet/model
    cloud/filestore/tools/analytics/libs/event-log

    library/cpp/eventlog/dumper
    library/cpp/getopt
)

SRCS(
    main.cpp
)

END()

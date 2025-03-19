LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    config.cpp
    critical_events.cpp
    filesystem_counters.cpp
    profile_log.cpp
    request_stats.cpp
    storage_counters.cpp
)

PEERDIR(
    cloud/filestore/config
    cloud/filestore/libs/diagnostics/events
    cloud/filestore/libs/service
    # FIXME use public api protos
    cloud/filestore/libs/storage/tablet/protos
    cloud/storage/core/libs/diagnostics
    cloud/storage/core/libs/version

    library/cpp/eventlog
    library/cpp/monlib/dynamic_counters
    library/cpp/monlib/service/pages
    library/cpp/protobuf/util
)

END()

RECURSE_FOR_TESTS(
    ut
)

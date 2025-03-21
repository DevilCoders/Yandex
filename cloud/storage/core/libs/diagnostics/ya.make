LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    critical_events.cpp
    histogram.cpp
    incomplete_requests.cpp
    logging.cpp
    max_calculator.cpp
    monitoring.cpp
    request_counters.cpp
    solomon_counters.cpp
    stats_updater.cpp
    trace_processor.cpp
    trace_serializer.cpp
    weighted_percentile.cpp
)

PEERDIR(
    cloud/storage/core/libs/common
    cloud/storage/core/protos

    library/cpp/lwtrace/mon

    library/cpp/actors/prof
    library/cpp/containers/ring_buffer
    library/cpp/deprecated/atomic
    library/cpp/histogram/hdr
    library/cpp/json/writer
    library/cpp/logger
    library/cpp/lwtrace
    library/cpp/monlib/dynamic_counters
    library/cpp/monlib/service
    library/cpp/monlib/service/pages
    library/cpp/monlib/service/pages/tablesorter

    logbroker/unified_agent/client/cpp/logger
)

END()

RECURSE_FOR_TESTS(ut)

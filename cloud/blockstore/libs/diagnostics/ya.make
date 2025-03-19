LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    block_digest.cpp
    cgroup_stats_fetcher.cpp
    config.cpp
    critical_events.cpp
    dumpable.cpp
    executor_counters.cpp
    fault_injection.cpp
    hostname.cpp
    probes.cpp
    profile_log.cpp
    quota_metrics.cpp
    request_stats.cpp
    server_stats.cpp
    server_stats_test.cpp
    stats_aggregator.cpp
    stats_helpers.cpp
    user_counter.cpp
    volume_perf.cpp
    volume_stats.cpp
    volume_stats_test.cpp
)

PEERDIR(
    cloud/blockstore/public/api/protos
    cloud/blockstore/config
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics/events
    cloud/blockstore/libs/service
    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics
    library/cpp/lwtrace/mon
    library/cpp/digest/crc32c
    library/cpp/eventlog
    library/cpp/histogram/hdr
    library/cpp/logger
    library/cpp/lwtrace
    library/cpp/monlib/dynamic_counters
    library/cpp/monlib/encode/spack
    library/cpp/monlib/service
    library/cpp/monlib/service/pages
    library/cpp/monlib/service/pages/tablesorter
    library/cpp/threading/hot_swap
    library/cpp/deprecated/atomic
)

END()

RECURSE_FOR_TESTS(
    ut
)

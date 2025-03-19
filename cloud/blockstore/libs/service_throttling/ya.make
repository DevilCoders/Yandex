LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    throttler_metrics.cpp
    throttler_policy.cpp
    throttler_tracker.cpp
    throttling.cpp
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/service
    cloud/blockstore/libs/throttling

    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics

    library/cpp/monlib/dynamic_counters
)

END()

RECURSE_FOR_TESTS(ut)

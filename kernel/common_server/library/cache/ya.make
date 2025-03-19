LIBRARY()

OWNER(g:cs_dev)

SRCS(
    cache.cpp
    cache_with_live_time.cpp
    caches_pool.cpp
)

PEERDIR(
    kernel/common_server/library/geometry/protos
    kernel/common_server/abstract
    kernel/common_server/proto
)

END()

RECURSE_FOR_TESTS(
)

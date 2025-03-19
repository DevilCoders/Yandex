PY3_PROGRAM()

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    yt/python/client

    cloud/blockstore/tools/analytics/find-perf-bottlenecks/lib
    cloud/blockstore/tools/analytics/find-perf-bottlenecks/ytlib
)

END()

RECURSE(
    lib
    ytlib
)

RECURSE_FOR_TESTS(
    tests
)

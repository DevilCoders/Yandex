PY3_PROGRAM()

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    yt/python/client

    cloud/blockstore/tools/analytics/filter-yt-logs/lib
)

END()

RECURSE(
    lib
)

RECURSE_FOR_TESTS(
    tests
)

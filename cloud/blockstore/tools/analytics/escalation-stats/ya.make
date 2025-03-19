PY3_PROGRAM()

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    yt/python/client

    cloud/blockstore/tools/analytics/escalation-stats/lib
)

END()

RECURSE(
    lib
)

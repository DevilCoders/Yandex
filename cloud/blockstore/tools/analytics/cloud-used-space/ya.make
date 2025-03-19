PY2_PROGRAM()

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/blockstore/public/sdk/python/client
    cloud/blockstore/tools/analytics/common

    contrib/python/requests
    contrib/python/tabulate
    contrib/python/progressbar2
    contrib/python/futures
)

END()

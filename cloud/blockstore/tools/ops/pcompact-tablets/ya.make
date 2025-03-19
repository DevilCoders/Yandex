PY2_PROGRAM(blockstore-pcompact-tablets)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/blockstore/public/sdk/python/client
    cloud/blockstore/tools/common/python

    contrib/python/requests
    contrib/python/progressbar2
    contrib/python/futures
)

PY_SRCS(
    __main__.py
)

END()

PY2_PROGRAM(blockstore-query-stats)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/blockstore/public/sdk/python/client
    cloud/blockstore/tools/common/python
)

PY_SRCS(
    __main__.py
)

END()

PY2_PROGRAM(blockstore-volume-storage-info)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/blockstore/public/sdk/python/client
    cloud/blockstore/tools/common/python
    contrib/python/requests
)

PY_SRCS(
    __main__.py
)

END()

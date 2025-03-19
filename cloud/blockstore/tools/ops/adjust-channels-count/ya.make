PY2_PROGRAM(blockstore-adjust-tablets-channels-count)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/blockstore/public/sdk/python/client
    cloud/blockstore/public/sdk/python/protos
)

PY_SRCS(
    __main__.py
)

END()

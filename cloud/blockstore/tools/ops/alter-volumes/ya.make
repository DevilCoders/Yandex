PY2_PROGRAM(blockstore-alter-volumes)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/blockstore/public/sdk/python/client
    cloud/blockstore/public/sdk/python/protos

    contrib/python/protobuf_to_dict
)

PY_SRCS(
    __main__.py
)

END()

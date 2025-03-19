PY2_PROGRAM(blockstore-prune-garbage-bulk)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/blockstore/tools/common/python

    contrib/python/futures
)

PY_SRCS(
    __main__.py
)

END()

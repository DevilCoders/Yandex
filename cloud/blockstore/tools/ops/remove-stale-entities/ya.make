PY3_PROGRAM(yc-remove-stale-entities)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/blockstore/tools/ops/remove-stale-entities/lib
)

END()

RECURSE(
    lib
)

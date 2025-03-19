PY3_PROGRAM(blockstore-patcher)

OWNER(g:cloud-nbs)

PEERDIR(
    cloud/blockstore/tools/cms/lib

    contrib/python/jsondiff
)

PY_SRCS(
    __main__.py
)

END()

RECURSE_FOR_TESTS(
    tests
)

PY3_PROGRAM(yc-nfs-multiclient-test)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/blockstore/pylibs/clusters
    cloud/blockstore/pylibs/common
    cloud/blockstore/pylibs/ycp
)

END()

RECURSE_FOR_TESTS(
    tests
)


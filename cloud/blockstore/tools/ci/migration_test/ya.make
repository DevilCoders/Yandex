PY3_PROGRAM(yc-nbs-ci-migration-test)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/blockstore/public/sdk/python/client
    cloud/blockstore/pylibs/clusters
    cloud/blockstore/pylibs/common
    cloud/blockstore/pylibs/sdk
    cloud/blockstore/pylibs/ycp
)

END()

RECURSE_FOR_TESTS(
    tests
)

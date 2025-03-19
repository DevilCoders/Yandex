PY3_PROGRAM(yc-nfs-ci-build-arcadia-test)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/blockstore/pylibs/clusters
    cloud/blockstore/pylibs/common
    cloud/blockstore/pylibs/ycp
)

RESOURCE(
    build.sh build.sh
)

END()

RECURSE_FOR_TESTS(
    tests
)

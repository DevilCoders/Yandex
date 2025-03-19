PY3_PROGRAM(yc-nbs-ci-corruption-test-suite)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/blockstore/pylibs/clients/solomon
    cloud/blockstore/pylibs/clusters
    cloud/blockstore/pylibs/common
    cloud/blockstore/pylibs/ycp

    cloud/blockstore/tools/ci/corruption_test_suite/lib
)

END()

RECURSE(
    lib
)

RECURSE_FOR_TESTS(
    tests
)

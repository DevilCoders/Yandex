PY3_PROGRAM(yc-nbs-ci-worst-case-read-performance-test-suite)

OWNER(g:cloud-nbs)

PY_SRCS(
    __main__.py
)

PEERDIR(
    cloud/blockstore/pylibs/clients/solomon
    cloud/blockstore/pylibs/clusters
    cloud/blockstore/pylibs/common
    cloud/blockstore/pylibs/ycp

    cloud/blockstore/tools/ci/worst_case_read_performance_test_suite/lib
)

END()

RECURSE(
    lib
)

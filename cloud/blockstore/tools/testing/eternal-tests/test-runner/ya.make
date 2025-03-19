PY3_PROGRAM(yc-nbs-run-eternal-load-tests)

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
    cloud/blockstore/tools/testing/eternal-tests/test-runner/lib
)

RESOURCE(
    configs/config.json test-config.json
    scripts/mysql.sh mysql.sh
    scripts/oltp-custom.lua oltp-custom.lua
    scripts/postgresql.sh postgresql.sh
)

END()

RECURSE(
    lib
)

RECURSE_FOR_TESTS(
    tests
)

TAG(
    ya:fat
    ya:force_sandbox
)

SIZE(LARGE)
TIMEOUT(3600)

REQUIREMENTS(
    cpu:8
    ram:32
    container:2185033214  # container with tcp_tw_reuse = 1
)

FORK_SUBTESTS()
SPLIT_FACTOR(5)

DEPENDS(
    cloud/blockstore/daemon
    cloud/blockstore/client
    cloud/blockstore/tools/testing/loadtest/bin

    kikimr/driver
    kikimr/public/tools/ydb
)

PEERDIR(
    cloud/blockstore/config
    cloud/blockstore/tests/python/lib

    kikimr/ci/libraries
    ydb/core/protos
)

TAG(
    ya:fat
    ya:force_sandbox
)

SIZE(LARGE)

TEST_SRCS(
    test.py
)

DEPENDS(
    cloud/blockstore/client
    cloud/blockstore/tools/testing/loadtest/bin

    cloud/storage/core/tools/testing/unstable-process

    kikimr/driver
)

DATA(
    arcadia/cloud/blockstore/tests/certs/server.crt
    arcadia/cloud/blockstore/tests/certs/server.key
    arcadia/cloud/blockstore/tests/loadtest/local-nonrepl
)

TIMEOUT(3600)

PEERDIR(
    cloud/blockstore/config
    cloud/blockstore/tests/python/lib

    kikimr/ci/libraries
    ydb/core/protos
)

REQUIREMENTS(
    ram_disk:16
    cpu:all
    container:2185033214  # container with tcp_tw_reuse = 1
)

PY3TEST()

OWNER(g:cloud-nbs)

TAG(
    ya:fat
)

TAG(
    ya:not_autocheck
    ya:manual
)

SIZE(LARGE)
TIMEOUT(3600)

REQUIREMENTS(
    cpu:8
    ram:32
)

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

TEST_SRCS(
    test.py
)

DATA(
    arcadia/cloud/blockstore/tests/loadtest/local-endpoints-spdk
)

SET(QEMU_PROC 8)
SET(QEMU_MEM 16G)
INCLUDE(${ARCADIA_ROOT}/cloud/storage/core/tests/recipes/qemu.inc)

END()

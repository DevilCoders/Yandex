PY3TEST()

OWNER(g:cloud-nbs)

TAG(
    ya:not_autocheck
    ya:manual
)

SIZE(MEDIUM)
TIMEOUT(600)

REQUIREMENTS(
    ram:8
)

DEPENDS(
    cloud/blockstore/libs/spdk/ut
)

TEST_SRCS(
    test.py
)

SET(QEMU_PROC 4)
SET(QEMU_MEM 8G)
INCLUDE(${ARCADIA_ROOT}/cloud/storage/core/tests/recipes/qemu.inc)

END()

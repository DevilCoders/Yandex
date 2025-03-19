PY3TEST()

OWNER(g:cloud-nbs)

TAG(
    ya:not_autocheck
    ya:manual
)

SIZE(MEDIUM)

DEPENDS(
    cloud/storage/core/libs/keyring/ut/bin
)

TEST_SRCS(
    test.py
)

SET(QEMU_ENABLE_KVM False)
INCLUDE(${ARCADIA_ROOT}/cloud/storage/core/tests/recipes/qemu.inc)

END()

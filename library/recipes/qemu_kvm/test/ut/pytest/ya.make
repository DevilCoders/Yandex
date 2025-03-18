OWNER(
    dmitko
    g:yatool
)

PY2TEST()

TEST_SRCS(
    test.py
)
# Use non default rootfs image
SET(QEMU_ROOTFS_DIR infra/environments/rtc-xenial/release/vm-image)
INCLUDE(${ARCADIA_ROOT}/library/recipes/qemu_kvm/recipe.inc)
END()

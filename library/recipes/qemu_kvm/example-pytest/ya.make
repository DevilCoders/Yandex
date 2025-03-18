PY2TEST()

OWNER(g:yatool dmitko)

TEST_SRCS(test.py)

PEERDIR(
    library/python/testing/pytest_runner
)
DATA(
    arcadia/devtools/dummy_arcadia/pytests-samples
)

# Use non default rootfs image
SET(QEMU_ROOTFS_DIR infra/environments/qavm-xenial/release/vm-image)
# Forward test execution to qemu-vm
INCLUDE(${ARCADIA_ROOT}/library/recipes/qemu_kvm/recipe.inc)
END()


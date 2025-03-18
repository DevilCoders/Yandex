PY2TEST()

OWNER(g:yatool dmitko)

TEST_SRCS(test.py)
# Forward test execution to qemu-vm
INCLUDE(${ARCADIA_ROOT}/library/recipes/qemu_kvm/recipe.inc)
END()

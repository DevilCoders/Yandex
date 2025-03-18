PY2TEST()

OWNER(g:yatool iaz1607)

TEST_SRCS(test.py)
REQUIREMENTS(ram_disk:4)
REQUIREMENTS(kvm)
# Forward test execution to qemu-vm
INCLUDE(${ARCADIA_ROOT}/library/recipes/qemu_kvm/recipe.inc)
END()

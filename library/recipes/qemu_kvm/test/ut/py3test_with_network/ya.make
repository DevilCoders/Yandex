OWNER(
    dmitko
    g:yatool
)

PY3TEST()

TEST_SRCS(
    conftest.py
    test.py
)
REQUIREMENTS(network:full)

INCLUDE(${ARCADIA_ROOT}/library/recipes/qemu_kvm/recipe.inc)
END()

OWNER(
    dmitko
    g:yatool
)

PY3TEST()

TEST_SRCS(
    test.py
)

INCLUDE(${ARCADIA_ROOT}/library/recipes/qemu_kvm/recipe.inc)
END()

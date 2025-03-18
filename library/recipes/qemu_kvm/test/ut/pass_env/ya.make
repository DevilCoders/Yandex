GO_TEST()

OWNER(g:yatool)

GO_TEST_SRCS(env_test.go)

INCLUDE(setenv/recipe.inc)
INCLUDE(${ARCADIA_ROOT}/library/recipes/qemu_kvm/recipe.inc)

END()

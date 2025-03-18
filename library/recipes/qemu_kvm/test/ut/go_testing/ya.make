OWNER(g:yatool)

GO_TEST()

GO_TEST_SRCS(
    main_test.go
)

INCLUDE(${ARCADIA_ROOT}/library/recipes/qemu_kvm/recipe.inc)

END()


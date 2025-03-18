GO_TEST()

OWNER(
    prime
    g:passport_infra
    g:go-library
)

ENV(GODEBUG="cgocheck=2")

INCLUDE(${ARCADIA_ROOT}/library/recipes/tvmapi/recipe.inc)

GO_TEST_SRCS(client_test.go)

END()

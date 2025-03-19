GO_TEST()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/deploydb/recipe/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/internal/dbteststeps/deps.inc)

SIZE(MEDIUM)

TIMEOUT(600)

GO_TEST_SRCS(
    func_test.go
    steps_test.go
)

END()

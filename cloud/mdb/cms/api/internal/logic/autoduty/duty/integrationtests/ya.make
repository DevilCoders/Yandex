GO_TEST()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/cms/db/recipe/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/internal/dbteststeps/deps.inc)

SIZE(MEDIUM)

GO_TEST_SRCS(
    duty_run_test.go
    helpers_test.go
)

END()

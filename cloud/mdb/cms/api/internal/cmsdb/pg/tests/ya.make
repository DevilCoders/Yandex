GO_TEST()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/cms/db/recipe/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/internal/dbteststeps/deps.inc)

SIZE(MEDIUM)

GO_XTEST_SRCS(
    pg_integration_test.go
    walle_pg_integration_test.go
)

END()

GO_TEST()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/mdb-vpc/db/recipe/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/internal/dbteststeps/deps.inc)

SIZE(MEDIUM)

GO_XTEST_SRCS(
    helpers_test.go
    network_connections_test.go
    networks_test.go
    operations_test.go
)

END()

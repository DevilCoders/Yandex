GO_TEST()

OWNER(g:mdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/katan/db/recipe/recipe.inc)

# dbaas_metadb + testdata

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/bare/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/testdata/recipe.inc)

# internal-api

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas-internal-api-image/recipe/recipe.inc)

DATA(arcadia/cloud/mdb/katan/functest)

SIZE(MEDIUM)

GO_TEST_SRCS(
    func_test.go
    katan_test.go
    monitoring_test.go
    scheduler_test.go
    steps_test.go
)

END()

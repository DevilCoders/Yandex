GO_TEST()

OWNER(g:mdb)

# dbaas_metadb + testdata

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/bare/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/testdata/recipe.inc)

# internal-api

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas-internal-api-image/recipe/recipe.inc)

DATA(arcadia/cloud/mdb/backup/functest)

SIZE(MEDIUM)

GO_TEST_SRCS(
    cli_test.go
    func_test.go
    steps_test.go
    worker_test.go
)

END()

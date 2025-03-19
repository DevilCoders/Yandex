GO_LIBRARY()

OWNER(g:mdb)

# dbaas_metadb + testdata

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/bare/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/testdata/recipe.inc)

# internal-api

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas-internal-api-image/recipe/recipe.inc)

DATA(arcadia/cloud/mdb/mdb-health/tests)

SIZE(MEDIUM)

SRCS(
    cluster_steps.go
    context.go
    rest_steps.go
)

GO_TEST_SRCS(func_test.go)

END()

RECURSE(gotest)

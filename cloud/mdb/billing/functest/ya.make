GO_TEST()

OWNER(g:mdb)

# billingdb + testdata

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/billing/db/recipe/recipe.inc)

# dbaas_metadb + testdata

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/bare/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/testdata/recipe.inc)

# internal-api

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas-internal-api-image/recipe/recipe.inc)

DATA(arcadia/cloud/mdb/billing/functest)

SIZE(MEDIUM)

GO_TEST_SRCS(
    func_test.go
    steps_test.go
)

END()

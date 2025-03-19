GO_TEST()

OWNER(g:mdb)

SIZE(MEDIUM)

# dbaas_metadb + testdata

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/bare/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/testdata/recipe.inc)

# internal-api

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas-internal-api-image/recipe/recipe.inc)

DATA(arcadia/cloud/mdb/mdb-maintenance/tests/features)

DATA(arcadia/cloud/mdb/mdb-maintenance/configs)

DATA(arcadia/cloud/mdb/mdb-maintenance/tests/certs)

GO_TEST_SRCS(
    cms_server_test.go
    context_test.go
    func_test.go
    steps_test.go
)

END()

RECURSE_FOR_TESTS(gotest)

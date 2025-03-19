GO_TEST()

OWNER(g:mdb)

# metadb recipe

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/bare/recipe.inc)

DEPENDS(cloud/mdb/mdb-search-producer/cmd/mdb-search-producer)

DATA(arcadia/cloud/mdb/mdb-search-producer/functest/features)

SIZE(medium)

GO_TEST_SRCS(
    func_test.go
    steps_test.go
)

END()

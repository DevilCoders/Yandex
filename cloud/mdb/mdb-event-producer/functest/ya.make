GO_TEST()

OWNER(g:mdb)

# metadb recipe

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/bare/recipe.inc)

DEPENDS(cloud/mdb/mdb-event-producer/cmd/mdb-event-producer)

DATA(arcadia/cloud/mdb/mdb-event-producer/functest/features)

SIZE(medium)

GO_TEST_SRCS(
    app_test.go
    steps_test.go
)

END()

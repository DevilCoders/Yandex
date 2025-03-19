GO_TEST()

OWNER(g:mdb)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/recipes/postgresql/recipe.inc)

USE_RECIPE(cloud/mdb/recipes/postgresql/postgresql_recipe)

DEPENDS(cloud/mdb/recipes/postgresql)

GO_XTEST_SRCS(tx_test.go)

END()

GO_TEST_FOR(cloud/mdb/mdb-reaper/internal/metadb/pg)

OWNER(g:mdb)

# dbaas_metadb + testdata

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/bare/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/testdata/recipe.inc)

# internal-api

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas-internal-api-image/recipe/recipe.inc)

SIZE(MEDIUM)

END()

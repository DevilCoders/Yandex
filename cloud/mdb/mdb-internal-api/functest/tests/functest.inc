INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/bare/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas_metadb/recipes/testdata/recipe.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbaas-internal-api-image/recipe/recipe.inc)

DATA(
    arcadia/cloud/mdb/mdb-internal-api/functest
    arcadia/cloud/search/schemas/v1
)


SIZE(MEDIUM)

REQUIREMENTS(
    cpu:4
    ram:8
    ram_disk:1
)

TIMEOUT(600)

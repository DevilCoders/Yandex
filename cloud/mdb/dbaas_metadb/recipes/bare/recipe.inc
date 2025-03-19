INCLUDE(${ARCADIA_ROOT}/cloud/mdb/recipes/postgresql/recipe.inc)

USE_RECIPE(cloud/mdb/dbaas_metadb/recipes/bare/metadb_bare_recipe)
DEPENDS(cloud/mdb/dbaas_metadb/recipes/bare)
DATA(arcadia/cloud/mdb/dbaas_metadb)

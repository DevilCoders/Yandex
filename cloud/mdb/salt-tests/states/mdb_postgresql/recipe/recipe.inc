INCLUDE(${ARCADIA_ROOT}/cloud/mdb/recipes/postgresql/recipe.inc)

USE_RECIPE(cloud/mdb/salt-tests/states/mdb_postgresql/recipe/mdb_postgresql_recipe)
DEPENDS(cloud/mdb/salt-tests/states/mdb_postgresql/recipe)
DATA(arcadia/cloud/mdb/salt-tests/states/mdb_postgresql)

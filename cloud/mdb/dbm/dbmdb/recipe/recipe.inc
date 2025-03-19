INCLUDE(${ARCADIA_ROOT}/cloud/mdb/recipes/postgresql/recipe.inc)

USE_RECIPE(cloud/mdb/dbm/dbmdb/recipe/dbm_recipe)
DEPENDS(cloud/mdb/dbm/dbmdb/recipe)
DATA(arcadia/cloud/mdb/dbm/dbmdb)

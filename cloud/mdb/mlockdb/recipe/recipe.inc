INCLUDE(${ARCADIA_ROOT}/cloud/mdb/recipes/postgresql/recipe.inc)

USE_RECIPE(cloud/mdb/mlockdb/recipe/mlockdb_recipe)
DEPENDS(cloud/mdb/mlockdb/recipe)
DATA(arcadia/cloud/mdb/mlockdb)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/recipes/postgresql/recipe.inc)

USE_RECIPE(cloud/mdb/deploydb/recipe/deploydb_recipe)
DEPENDS(cloud/mdb/deploydb/recipe)
DATA(arcadia/cloud/mdb/deploydb)

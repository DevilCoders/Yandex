INCLUDE(${ARCADIA_ROOT}/cloud/mdb/recipes/postgresql/recipe.inc)

USE_RECIPE(cloud/mdb/katan/db/recipe/katandb_recipe)
DEPENDS(cloud/mdb/katan/db/recipe)
DATA(arcadia/cloud/mdb/katan/db)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/recipes/postgresql/recipe.inc)

USE_RECIPE(cloud/mdb/billing/db/recipe/billingdb_recipe)
DEPENDS(cloud/mdb/billing/db/recipe)
DATA(arcadia/cloud/mdb/billing/db)

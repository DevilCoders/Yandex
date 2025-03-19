INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbm/dbmdb/recipe/recipe.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbm/tests/recipes/deploy-api/recipe.inc)
INCLUDE(${ARCADIA_ROOT}/cloud/mdb/dbm/tests/recipes/passport/recipe.inc)

USE_RECIPE(cloud/mdb/dbm/tests/recipes/dbm/dbm-recipe)
DEPENDS(cloud/mdb/dbm/tests/recipes/dbm)
DATA(arcadia/cloud/mdb/dbm/app.yaml)

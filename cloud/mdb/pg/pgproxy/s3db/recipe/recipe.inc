INCLUDE(${ARCADIA_ROOT}/cloud/mdb/recipes/postgresql/recipe.inc)

USE_RECIPE(cloud/mdb/pg/pgproxy/s3db/recipe/s3db_recipe)
DEPENDS(cloud/mdb/pg/pgproxy/s3db/recipe)
DATA(arcadia/cloud/mdb/pg)

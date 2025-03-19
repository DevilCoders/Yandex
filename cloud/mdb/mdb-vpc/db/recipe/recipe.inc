INCLUDE(${ARCADIA_ROOT}/cloud/mdb/recipes/postgresql/recipe.inc)

USE_RECIPE(cloud/mdb/mdb-vpc/db/recipe/vpcdb_recipe)
DEPENDS(cloud/mdb/mdb-vpc/db/recipe)
DATA(arcadia/cloud/mdb/mdb-vpc/db)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/recipes/postgresql/recipe.inc)

USE_RECIPE(cloud/mdb/recipes/postgresql-replica/postgresql_replica_recipe)
DEPENDS(cloud/mdb/recipes/postgresql-replica)

INCLUDE(${ARCADIA_ROOT}/cloud/mdb/recipes/postgresql/recipe.inc)

USE_RECIPE(cloud/mdb/cms/db/recipe/cms_recipe)
DEPENDS(cloud/mdb/cms/db/recipe)
DATA(arcadia/cloud/mdb/cms/db)

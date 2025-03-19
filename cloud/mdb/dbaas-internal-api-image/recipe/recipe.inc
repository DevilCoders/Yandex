USE_RECIPE(cloud/mdb/dbaas-internal-api-image/recipe/dbaas_internal_api_recipe)
DEPENDS(cloud/mdb/dbaas-internal-api-image)
DEPENDS(cloud/mdb/dbaas-internal-api-image/recipe)

DATA(arcadia/cloud/mdb/dbaas-internal-api-image)

REQUIREMENTS(
    ram_disk:1
)

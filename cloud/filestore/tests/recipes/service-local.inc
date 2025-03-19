DEPENDS(
    cloud/filestore/server
    cloud/filestore/tests/recipes/service-local
)

REQUIREMENTS(
    ram_disk:1
)

USE_RECIPE(
    cloud/filestore/tests/recipes/service-local/service-local-recipe --service=local
)

DEPENDS(
    cloud/filestore/server
    cloud/filestore/tests/recipes/service-local
)

USE_RECIPE(
    cloud/filestore/tests/recipes/service-local/service-local-recipe --service=null
)

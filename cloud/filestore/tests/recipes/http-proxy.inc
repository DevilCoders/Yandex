DEPENDS(
    cloud/filestore/tools/http_proxy
    cloud/filestore/tests/recipes/http-proxy
)

USE_RECIPE(
    cloud/filestore/tests/recipes/http-proxy/http-proxy-recipe
)

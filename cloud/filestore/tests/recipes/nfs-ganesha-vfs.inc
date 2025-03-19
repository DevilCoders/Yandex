DEPENDS(
    cloud/filestore/gateway/nfs/server
    cloud/filestore/tests/recipes/nfs-ganesha
)

USE_RECIPE(
    cloud/filestore/tests/recipes/nfs-ganesha/nfs-ganesha-recipe --service vfs
)

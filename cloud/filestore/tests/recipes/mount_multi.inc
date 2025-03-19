DEPENDS(
    cloud/filestore/client
    cloud/filestore/tests/recipes/mount
)

USE_RECIPE(
    cloud/filestore/tests/recipes/mount/mount-recipe 
        --filesystem nfs_share 
        --mount-path nfs_mount1 
        --mount-path nfs_mount2 
        --mount-path nfs_mount3 
        --verbose
)

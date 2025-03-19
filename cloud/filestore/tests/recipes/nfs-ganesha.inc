DEPENDS(
    cloud/filestore/client
    cloud/filestore/gateway/nfs/server
    cloud/filestore/tests/recipes/nfs-ganesha
)

IF (GANESHA_RESTART_INTERVAL)
    DEPENDS(
        cloud/storage/core/tools/testing/unstable-process
    )
ENDIF()

USE_RECIPE(
    cloud/filestore/tests/recipes/nfs-ganesha/nfs-ganesha-recipe
        --restart-interval $GANESHA_RESTART_INTERVAL
        --restart-flag  $GANESHA_RESTART_FLAG
        --verbose
)

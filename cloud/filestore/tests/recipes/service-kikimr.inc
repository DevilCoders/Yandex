DEPENDS(
    cloud/filestore/server
    cloud/filestore/tests/recipes/service-kikimr

    kikimr/driver
)

IF (NFS_RESTART_INTERVAL)
    DEPENDS(
        cloud/storage/core/tools/testing/unstable-process
    )
ENDIF()

USE_RECIPE(
    cloud/filestore/tests/recipes/service-kikimr/service-kikimr-recipe 
        --use-log-files 
        --in-memory-pdisks 
        --restart-interval $NFS_RESTART_INTERVAL 
        --verbose
)

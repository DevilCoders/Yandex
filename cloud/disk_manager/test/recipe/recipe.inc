SET(RECIPE_DEPS_LIST
    cloud/filestore/server

    cloud/compute/snapshot/cmd/yc-snapshot
    cloud/compute/snapshot/cmd/yc-snapshot-populate-database

    cloud/disk_manager/cmd/yc-disk-manager
    cloud/disk_manager/cmd/yc-disk-manager-init-db
    cloud/disk_manager/test/images/server
    cloud/disk_manager/test/mocks/accessservice
    cloud/disk_manager/test/mocks/metadata
    cloud/disk_manager/test/nemesis
    cloud/disk_manager/test/recipe

    contrib/python/moto/bin
    library/recipes/s3mds
)

IF (STABLE_DEPS_RECIPE)
    SET_APPEND(RECIPE_ARGS stable)
    SET_APPEND(RECIPE_DEPS_LIST
        kikimr/public/tools/package/stable
        cloud/blockstore/tests/recipes/local-kikimr/stable-package-nbs
    )
ELSE()
    SET_APPEND(RECIPE_DEPS_LIST
        kikimr/driver
        cloud/blockstore/daemon
    )
ENDIF()

DEPENDS(${RECIPE_DEPS_LIST})

DATA(
    arcadia/cloud/blockstore/tests/certs/server.crt
    arcadia/cloud/blockstore/tests/certs/server.key
    arcadia/cloud/disk_manager/test/images/image.img
)

USE_RECIPE(cloud/disk_manager/test/recipe/recipe ${RECIPE_ARGS})

USE_RECIPE(library/recipes/s3mds/s3mds-recipe)

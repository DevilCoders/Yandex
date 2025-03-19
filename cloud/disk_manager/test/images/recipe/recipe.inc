SET(RECIPE_DEPS_LIST
    cloud/disk_manager/test/images/server
    cloud/disk_manager/test/images/recipe
)

DEPENDS(${RECIPE_DEPS_LIST})

DATA(
    arcadia/cloud/disk_manager/test/images/image.img
    arcadia/cloud/disk_manager/test/images/invalid_image.img
)

USE_RECIPE(cloud/disk_manager/test/images/recipe/recipe)

DEPENDS(
    contrib/tools/jdk
    library/recipes/zookeeper
    library/recipes/zookeeper/package
)

USE_RECIPE(library/recipes/zookeeper/zookeeper "${ZOOKEEPER_RECIPE_OPTS}")

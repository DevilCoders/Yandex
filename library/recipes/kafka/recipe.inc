BUILD_ONLY_IF(OS_LINUX ARCH_X86_64)

DATA(sbr://2513115393)  # kafka

DEPENDS(
    contrib/tools/jdk
    library/recipes/kafka
)

INCLUDE(${ARCADIA_ROOT}/library/recipes/zookeeper/recipe.inc)

USE_RECIPE(library/recipes/kafka/kafka_recipe)

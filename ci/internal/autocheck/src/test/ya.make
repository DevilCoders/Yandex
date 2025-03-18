JUNIT5()

SIZE(MEDIUM)

JAVA_SRCS(SRCDIR java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

PEERDIR(
    ci/internal/ci/engine/src/test
    ci/internal/integration-tests/storage-observer-tests/src/test
)

END()

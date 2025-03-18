JUNIT5()

SIZE(MEDIUM)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

PEERDIR(
    ci/internal/ci/flow-engine
    ci/internal/ci/core/src/test
    ci/internal/ci/flow-engine/src/test/proto
    ci/internal/common/bazinga/src/test
    ci/internal/common/temporal/src/test
)


END()

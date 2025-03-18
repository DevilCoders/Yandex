JUNIT5()

SIZE(MEDIUM)
FORK_SUBTESTS()
SPLIT_FACTOR(4)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

PEERDIR(
    ci/internal/ci/api
    ci/internal/ci/engine/src/test
)

END()

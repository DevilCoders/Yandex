JUNIT5()

SIZE(MEDIUM)
FORK_SUBTESTS()
SPLIT_FACTOR(16)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

PEERDIR(
    ci/internal/ci/tms

    ci/internal/ci/engine/src/test
    ci/internal/common/temporal/src/test
    ci/internal/common/bazinga/src/test

    ci/tasklet/registry/demo/woodflow/woodcutter/proto
    ci/tasklet/registry/demo/woodflow/sawmill/proto
    ci/tasklet/registry/demo/woodflow/furniture_factory/proto
    ci/tasklet/registry/demo/picklock/proto

)

END()

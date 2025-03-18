JUNIT5()

SIZE(MEDIUM)
FORK_SUBTESTS()
SPLIT_FACTOR(8)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

DATA(arcadia/autocheck/autocheck.yaml)
DATA(arcadia/autocheck/balancing_configs/autocheck-linux.json)

PEERDIR(
    ci/internal/ci/engine
    ci/internal/ci/flow-engine/src/test
    ci/internal/common/bazinga/src/test
    ci/internal/common/temporal/src/test

    ci/clients/arcanum/src/test
    ci/clients/storage/src/test
)

END()

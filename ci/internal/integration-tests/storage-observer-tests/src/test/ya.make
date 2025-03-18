JUNIT5()

SIZE(MEDIUM)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

PEERDIR(
    ci/internal/integration-tests/storage-observer-tests
    ci/internal/ci/core
    ci/internal/observer/reader
    ci/internal/observer/api
    ci/internal/storage/tms
    ci/internal/storage/api

    # tests
    ci/internal/observer/api/src/test
    ci/internal/observer/reader/src/test

    ci/internal/storage/tests/src/test
)

END()

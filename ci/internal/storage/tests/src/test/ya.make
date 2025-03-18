JUNIT5()

SIZE(MEDIUM)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

PEERDIR(
    ci/internal/storage/tests

    # tests
    ci/internal/storage/core/src/test
    ci/internal/storage/api/src/test
    ci/internal/storage/reader/src/test
    ci/internal/storage/shard/src/test
    ci/internal/storage/post-processor/src/test
    ci/internal/storage/tms/src/test

    yt/java/ytclient/testlib
)

END()

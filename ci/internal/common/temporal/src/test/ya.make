JUNIT5()

SIZE(MEDIUM)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

PEERDIR(
    ci/clients/grpc/src/test
    ci/clients/logs/src/test

    ci/internal/common/temporal
    ci/internal/common/ydb/src/test

    contrib/java/org/springframework/boot/spring-boot-starter-test
    contrib/java/io/temporal/temporal-testing
    contrib/java/org/reflections/reflections
    contrib/java/io/github/benas/random-beans
    contrib/java/org/awaitility/awaitility
)

END()

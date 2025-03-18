JUNIT5()

SIZE(MEDIUM)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

PEERDIR(
    ci/internal/ci/core

    ci/internal/ci/core/src/test/proto

    ci/clients/abc/src/test
    ci/clients/grpc/src/test
    ci/clients/taskletv2/src/test

    ci/internal/common/ydb/src/test

    contrib/java/io/github/benas/random-beans
    contrib/java/org/springframework/boot/spring-boot-starter-test

    contrib/java/org/javassist/javassist
)

DATA(arcadia/ci/proto)

END()

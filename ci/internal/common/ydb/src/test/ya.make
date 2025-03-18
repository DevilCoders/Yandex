JUNIT5()

SIZE(MEDIUM)

JAVA_SRCS(SRCDIR java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

PEERDIR(
    ci/internal/common/ydb
    contrib/java/org/springframework/boot/spring-boot-starter-test
)

END()

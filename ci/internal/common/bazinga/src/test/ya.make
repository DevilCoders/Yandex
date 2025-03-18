JUNIT5()

SIZE(SMALL)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)

PEERDIR(
    ci/internal/common/bazinga

    contrib/java/org/apache/curator/curator-test
    contrib/java/org/springframework/boot/spring-boot-starter-test
)

END()

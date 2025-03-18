JUNIT5()

SIZE(SMALL)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)

PEERDIR(
    ci/internal/ayamler/api
    ci/internal/ci/core/src/test
)

END()

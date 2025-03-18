JUNIT5()

SIZE(SMALL)

JAVA_SRCS(SRCDIR java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)

PEERDIR(
    ci/clients/logs
)

END()

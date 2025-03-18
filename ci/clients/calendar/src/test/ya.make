JUNIT5()


OWNER(g:ci)

SIZE(SMALL)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)

PEERDIR(
    ci/clients/calendar
)

END()

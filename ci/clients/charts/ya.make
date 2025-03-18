JAVA_LIBRARY(charts-client)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/clients/http-client-base
)

END()

RECURSE_FOR_TESTS(src/test)

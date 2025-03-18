JAVA_LIBRARY(arcanum-client)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/clients/http-client-base
    ci/clients/tvm
)

END()

RECURSE_FOR_TESTS(src/test)


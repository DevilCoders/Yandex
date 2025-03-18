JAVA_LIBRARY(sandbox-client)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/clients/http-client-base
    ci/clients/tvm

    contrib/java/javax/xml/bind/jaxb-api
)

END()

RECURSE_FOR_TESTS(src/test)

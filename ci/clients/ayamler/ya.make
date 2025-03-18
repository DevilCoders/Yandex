JAVA_LIBRARY(ayamler-client)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/proto/ayamler

    ci/clients/grpc
    library/java/annotations
)

END()

RECURSE_FOR_TESTS(src/test)

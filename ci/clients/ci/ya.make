JAVA_LIBRARY(ci-client)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/proto
    ci/clients/grpc
)

END()

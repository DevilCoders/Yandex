JAVA_LIBRARY(logs-client)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/clients/grpc
    devtools/observability/logging/vtail/api/query
)

END()

RECURSE_FOR_TESTS(src/test)

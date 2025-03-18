JAVA_LIBRARY(taskletv2-client)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/clients/grpc
    tasklet/api/v2
    tasklet/sdk/v2/java
)

END()

RECURSE_FOR_TESTS(
    src/test
)


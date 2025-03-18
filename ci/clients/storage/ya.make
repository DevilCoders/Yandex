JAVA_LIBRARY(storage-client)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/proto/storage

    ci/clients/grpc
    library/java/annotations
)

END()

RECURSE_FOR_TESTS(src/test)

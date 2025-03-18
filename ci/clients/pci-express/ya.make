JAVA_LIBRARY(pci-express-client)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    repo/pciexpress/proto_api

    ci/clients/grpc
    library/java/annotations
)

END()

RECURSE_FOR_TESTS(src/test)

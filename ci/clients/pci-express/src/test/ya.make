JUNIT5()


OWNER(g:ci)

SIZE(SMALL)

JAVA_SRCS(SRCDIR java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes-test.inc)

PEERDIR(
    ci/clients/pci-express
    ci/clients/grpc/src/test
)

END()

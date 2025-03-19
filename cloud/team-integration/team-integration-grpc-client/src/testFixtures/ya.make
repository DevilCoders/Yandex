OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.testFixtures.inc)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    cloud/team-integration/team-integration-grpc-client

    contrib/java/io/grpc/grpc-api
    contrib/java/io/grpc/grpc-stub
)

END()

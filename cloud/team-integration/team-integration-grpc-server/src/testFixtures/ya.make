OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.testFixtures.inc)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    cloud/team-integration/team-integration-grpc-server

    contrib/java/io/grpc/grpc-api
    contrib/java/yandex/cloud/common/library/util
)

END()

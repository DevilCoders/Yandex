OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.test.inc)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    cloud/team-integration/team-integration-team-abcd-client
    cloud/team-integration/team-integration-team-abcd-client/src/testFixtures

    cloud/team-integration/team-integration-grpc-client/src/testFixtures

    contrib/java/io/grpc/grpc-api
    contrib/java/io/grpc/grpc-core
    contrib/java/io/grpc/grpc-testing
    contrib/java/yandex/cloud/common/library/json-util
    intranet/d/backend/service/proto
)

END()

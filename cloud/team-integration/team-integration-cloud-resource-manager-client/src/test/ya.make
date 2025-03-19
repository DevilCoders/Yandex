OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.test.inc)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    cloud/team-integration/team-integration-cloud-resource-manager-client
    cloud/team-integration/team-integration-cloud-resource-manager-client/src/testFixtures

    cloud/team-integration/team-integration-grpc-client/src/testFixtures

    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/io/grpc/grpc-api
    contrib/java/io/grpc/grpc-core
    contrib/java/io/grpc/grpc-testing
    contrib/java/ru/yandex/cloud/cloud-proto-java
    contrib/java/yandex/cloud/common/library/json-util
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/iam-remote-operation
)

END()

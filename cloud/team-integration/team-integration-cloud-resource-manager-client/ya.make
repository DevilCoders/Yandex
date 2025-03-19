OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-cloud-resource-manager-client)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    cloud/team-integration/team-integration-grpc-client

    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/io/grpc/grpc-api
    contrib/java/io/grpc/grpc-stub
    contrib/java/ru/yandex/cloud/cloud-proto-java
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/iam-remote-operation
)

END()

RECURSE_FOR_TESTS(
    src/test
    src/testFixtures
)

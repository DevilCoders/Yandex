OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-abc)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    cloud/team-integration/team-integration-cloud-billing-client
    cloud/team-integration/team-integration-cloud-resource-manager-client
    cloud/team-integration/team-integration-grpc-client
    cloud/team-integration/team-integration-grpc-server
    cloud/team-integration/team-integration-http-client
    cloud/team-integration/team-integration-legacy
    cloud/team-integration/team-integration-operation
    cloud/team-integration/team-integration-team-abc-client
    cloud/team-integration/team-integration-team-abcd-client

    contrib/java/com/google/api/grpc/proto-google-common-protos
    contrib/java/com/google/guava/guava
    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/io/grpc/grpc-api
    contrib/java/io/grpc/grpc-stub
    contrib/java/io/prometheus/simpleclient_guava
    contrib/java/javax/inject/javax.inject
    contrib/java/org/apache/logging/log4j/log4j-api
    contrib/java/ru/yandex/cloud/cloud-proto-java
    contrib/java/yandex/cloud/common/library/json-util
    contrib/java/yandex/cloud/common/library/repository-core
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/common/library/task-processor
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/iam-common-exception
    contrib/java/yandex/cloud/iam-common
    contrib/java/yandex/cloud/iam-remote-operation
    contrib/java/yandex/cloud/operations
)

END()

RECURSE_FOR_TESTS(
    src/test
    src/testFixtures
)

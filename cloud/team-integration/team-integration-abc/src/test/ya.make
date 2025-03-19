OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.test.inc)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    cloud/team-integration/team-integration-abc
    cloud/team-integration/team-integration-abc/src/testFixtures

    cloud/team-integration/team-integration-cloud-billing-client/src/testFixtures
    cloud/team-integration/team-integration-cloud-resource-manager-client/src/testFixtures
    cloud/team-integration/team-integration-grpc-server/src/testFixtures
    cloud/team-integration/team-integration-http-server/src/testFixtures
    cloud/team-integration/team-integration-repository/src/testFixtures
    cloud/team-integration/team-integration-team-abc-client/src/testFixtures
    cloud/team-integration/team-integration-team-abcd-client/src/testFixtures
    cloud/team-integration/team-integration-tvm-client/src/testFixtures

    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/io/grpc/grpc-api
    contrib/java/io/grpc/grpc-core
    contrib/java/io/grpc/grpc-stub
    contrib/java/io/prometheus/simpleclient
    contrib/java/io/prometheus/simpleclient_guava
    contrib/java/javax/inject/javax.inject
    contrib/java/ru/yandex/cloud/cloud-proto-java
    contrib/java/yandex/cloud/iam/access-service-client-api
    contrib/java/yandex/cloud/common/dependencies/jetty-application
    contrib/java/yandex/cloud/common/library/json-util
    contrib/java/yandex/cloud/common/library/metrics
    contrib/java/yandex/cloud/common/library/operation-client-test
    contrib/java/yandex/cloud/common/library/repository-core
    contrib/java/yandex/cloud/common/library/scenario
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/common/library/task-processor
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/fake-cloud-core
    contrib/java/yandex/cloud/iam-common-test
    contrib/java/yandex/cloud/iam-common
    contrib/java/yandex/cloud/iam-remote-operation
    contrib/java/yandex/cloud/operations
)

END()

OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.test.inc)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

PEERDIR(
    cloud/team-integration/team-integration-abcd-adapter
    cloud/team-integration/team-integration-abcd-adapter/src/testFixtures

    cloud/team-integration/team-integration-abc/src/testFixtures
    cloud/team-integration/team-integration-abcd-provider-api/src/testFixtures
    cloud/team-integration/team-integration-grpc-server/src/testFixtures
    cloud/team-integration/team-integration-team-abcd-client/src/testFixtures
    cloud/team-integration/team-integration-tvm-client/src/testFixtures

    contrib/java/io/grpc/grpc-api
    contrib/java/io/grpc/grpc-core
    contrib/java/io/grpc/grpc-stub
    contrib/java/io/prometheus/simpleclient
    contrib/java/javax/inject/javax.inject
    contrib/java/yandex/cloud/iam/access-service-client-api
    contrib/java/yandex/cloud/common/library/lang-test
    contrib/java/yandex/cloud/common/library/scenario
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/fake-cloud-core
    contrib/java/yandex/cloud/iam-common-tvm
    contrib/java/yandex/cloud/iam-common
    intranet/d/backend/service/provider_proto
)

END()

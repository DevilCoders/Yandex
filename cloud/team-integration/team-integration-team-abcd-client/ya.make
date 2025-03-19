OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-team-abcd-client)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    cloud/team-integration/team-integration-grpc-client
    cloud/team-integration/team-integration-team-abc-client
    cloud/team-integration/team-integration-tvm-grpc

    contrib/java/io/grpc/grpc-api
    contrib/java/io/grpc/grpc-stub
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/iam-common-exception
    contrib/java/yandex/cloud/iam-common-tvm
    contrib/java/yandex/cloud/iam-common
    contrib/java/yandex/cloud/iam-remote-operation
    intranet/d/backend/service/proto
)

END()

RECURSE_FOR_TESTS(
    src/test
    src/testFixtures
)

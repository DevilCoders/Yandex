OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-abcd-adapter)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    cloud/team-integration/team-integration-abc
    cloud/team-integration/team-integration-abcd-provider-api
    cloud/team-integration/team-integration-abcd-provider-impl/team-integration-abcd-provider-mdb-impl
    cloud/team-integration/team-integration-abcd-provider-impl/team-integration-abcd-provider-ydb-impl
    cloud/team-integration/team-integration-grpc-client
    cloud/team-integration/team-integration-team-abc-client
    cloud/team-integration/team-integration-team-abcd-client

    contrib/java/io/grpc/grpc-api
    contrib/java/io/grpc/grpc-context
    contrib/java/io/grpc/grpc-stub
    contrib/java/org/apache/logging/log4j/log4j-api
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/iam-common-exception
    contrib/java/yandex/cloud/iam-common
    intranet/d/backend/service/provider_proto
)

END()

RECURSE_FOR_TESTS(
    src/test
    src/testFixtures
)

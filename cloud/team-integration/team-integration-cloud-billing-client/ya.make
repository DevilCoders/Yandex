OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-cloud-billing-client)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    cloud/team-integration/team-integration-http-client

    contrib/java/com/google/api/grpc/proto-google-common-protos
    contrib/java/io/grpc/grpc-api
    contrib/java/javax/inject/javax.inject
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/iam-remote-operation
    contrib/java/yandex/cloud/operations
)

END()

RECURSE_FOR_TESTS(
    src/test
    src/testFixtures
)

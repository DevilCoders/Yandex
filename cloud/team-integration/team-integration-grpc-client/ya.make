OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-grpc-client)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    contrib/java/io/grpc/grpc-api
    contrib/java/io/grpc/grpc-context
    contrib/java/io/grpc/grpc-core
    contrib/java/io/grpc/grpc-netty
    contrib/java/io/grpc/grpc-stub
    contrib/java/yandex/cloud/common/library/util
)

END()

RECURSE_FOR_TESTS(
    src/testFixtures
)

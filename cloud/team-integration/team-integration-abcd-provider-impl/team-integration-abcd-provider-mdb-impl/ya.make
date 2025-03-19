OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-abcd-provider-mdb-impl)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    cloud/team-integration/team-integration-abcd-provider-api
    cloud/team-integration/team-integration-grpc-client

    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/io/grpc/grpc-api
    contrib/java/io/grpc/grpc-stub
    contrib/java/ru/yandex/cloud/cloud-proto-java
    contrib/java/yandex/cloud/common/library/util
)

END()

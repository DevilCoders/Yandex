OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.testFixtures.inc)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    cloud/team-integration/team-integration-cloud-resource-manager-client

    contrib/java/com/google/protobuf/protobuf-java
    contrib/java/ru/yandex/cloud/cloud-proto-java
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/iam-remote-operation
)

END()

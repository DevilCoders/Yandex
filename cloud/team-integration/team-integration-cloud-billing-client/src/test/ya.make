OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.test.inc)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    cloud/team-integration/team-integration-cloud-billing-client
    cloud/team-integration/team-integration-cloud-billing-client/src/testFixtures
    cloud/team-integration/team-integration-http-server/src/testFixtures

    contrib/java/io/grpc/grpc-api
    contrib/java/javax/inject/javax.inject
    contrib/java/yandex/cloud/common/dependencies/jetty-application
    contrib/java/yandex/cloud/common/library/scenario
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/fake-cloud-core
    contrib/java/yandex/cloud/iam-remote-operation
)

END()

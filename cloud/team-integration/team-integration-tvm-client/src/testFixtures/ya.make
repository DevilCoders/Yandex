OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.testFixtures.inc)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    cloud/team-integration/team-integration-tvm-client

    contrib/java/yandex/cloud/common/dependencies/jetty-application-tests
    contrib/java/yandex/cloud/common/library/operation-client-test
    contrib/java/yandex/cloud/common/library/scenario
    contrib/java/yandex/cloud/fake-cloud-core
    contrib/java/yandex/cloud/operations
)

END()

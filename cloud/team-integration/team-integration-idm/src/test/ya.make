OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.test.inc)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

PEERDIR(
    cloud/team-integration/team-integration-idm

    cloud/team-integration/team-integration-abc/src/testFixtures
    cloud/team-integration/team-integration-cloud-resource-manager-client/src/testFixtures
    cloud/team-integration/team-integration-http-server/src/testFixtures

    contrib/java/yandex/cloud/common/library/operation-client-test
    contrib/java/yandex/cloud/common/library/scenario
    contrib/java/yandex/cloud/fake-cloud-core
    contrib/java/yandex/cloud/fake-cloud-core-tests
    contrib/java/yandex/cloud/iam-common-test
)

END()

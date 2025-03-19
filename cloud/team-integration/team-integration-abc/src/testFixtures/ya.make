OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.testFixtures.inc)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    cloud/team-integration/team-integration-abc

    cloud/team-integration/team-integration-team-abcd-client/src/testFixtures

    contrib/java/yandex/cloud/operations
)

END()

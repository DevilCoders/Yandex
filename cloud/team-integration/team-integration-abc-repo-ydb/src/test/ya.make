OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.test.inc)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

PEERDIR(
    cloud/team-integration/team-integration-abc-repo-ydb

    cloud/team-integration/team-integration-abc/src/testFixtures
    cloud/team-integration/team-integration-repository/src/testFixtures

    contrib/java/yandex/cloud/iam-common-test
)

END()

OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.test.inc)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    cloud/team-integration/team-integration-abcd-provider-api
    cloud/team-integration/team-integration-abcd-provider-api/src/testFixtures
)

END()

OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.testFixtures.inc)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    cloud/team-integration/team-integration-team-abc-client

    contrib/java/yandex/cloud/common/library/static-di
)

END()

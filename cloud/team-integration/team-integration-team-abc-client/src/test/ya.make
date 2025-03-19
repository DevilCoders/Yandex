OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.test.inc)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    cloud/team-integration/team-integration-team-abc-client
    cloud/team-integration/team-integration-team-abc-client/src/testFixtures

    cloud/team-integration/team-integration-http-client

    contrib/java/com/squareup/okhttp3/mockwebserver
    contrib/java/yandex/cloud/common/library/json-util
    contrib/java/yandex/cloud/iam-common-tvm
)

END()

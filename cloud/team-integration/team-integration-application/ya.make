OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-application)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

PEERDIR(
    cloud/team-integration/team-integration-abc-repo-ydb
    cloud/team-integration/team-integration-abcd-adapter
    cloud/team-integration/team-integration-http-server
    cloud/team-integration/team-integration-idm
    cloud/team-integration/team-integration-tvm-client

    contrib/java/yandex/cloud/cloud-auth-config
    contrib/java/yandex/cloud/common/dependencies/jetty-application
    contrib/java/yandex/cloud/common/library/application
)

END()

RECURSE_FOR_TESTS(
    src/test
)

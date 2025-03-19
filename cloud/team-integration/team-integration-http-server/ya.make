OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-http-server)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    contrib/java/javax/servlet/javax.servlet-api
    contrib/java/org/eclipse/jetty/jetty-security
    contrib/java/yandex/cloud/common/dependencies/jetty-application
    contrib/java/yandex/cloud/common/library/static-di
)

END()

RECURSE_FOR_TESTS(
    src/testFixtures
)

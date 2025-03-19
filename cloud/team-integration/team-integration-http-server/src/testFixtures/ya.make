OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.testFixtures.inc)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    cloud/team-integration/team-integration-http-server

    contrib/java/javax/servlet/javax.servlet-api
    contrib/java/org/eclipse/jetty/jetty-security
    contrib/java/yandex/cloud/common/dependencies/jetty-application
)

END()

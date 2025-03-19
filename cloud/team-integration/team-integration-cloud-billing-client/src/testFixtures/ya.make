OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.testFixtures.inc)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

PEERDIR(
    cloud/team-integration/team-integration-cloud-billing-client

    contrib/java/javax/servlet/javax.servlet-api
    contrib/java/yandex/cloud/common/dependencies/jetty-application
    contrib/java/yandex/cloud/common/dependencies/jetty-application-tests
)

END()

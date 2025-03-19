OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-tvm-http)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    contrib/java/javax/servlet/javax.servlet-api
    contrib/java/org/apache/logging/log4j/log4j-api
    contrib/java/org/eclipse/jetty/jetty-security
    contrib/java/org/eclipse/jetty/jetty-server
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/iam-common-tvm
)

END()

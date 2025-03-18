JAVA_LIBRARY(ci-common-application)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/common/application-profiles

    contrib/java/org/springframework/boot/spring-boot-starter-web
    contrib/java/org/springframework/boot/spring-boot-starter-jetty
    contrib/java/org/springframework/boot/spring-boot-starter-actuator

    contrib/java/org/springframework/boot/spring-boot-starter-log4j2
    contrib/java/org/apache/logging/log4j/log4j-web
    contrib/java/org/apache/logging/log4j/log4j-1.2-api
)

END()

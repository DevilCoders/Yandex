JAVA_LIBRARY(ci-common-logbroker)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/common/utils

    kikimr/persqueue/sdk/java/v0

    contrib/java/org/springframework/spring-context
    contrib/java/io/micrometer/micrometer-registry-prometheus
)

END()


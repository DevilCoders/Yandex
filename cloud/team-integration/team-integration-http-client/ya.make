OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-http-client)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/iam-common
)

END()

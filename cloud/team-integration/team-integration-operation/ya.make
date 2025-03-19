OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-operation)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    contrib/java/yandex/cloud/common/library/repository-core
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/operations
)

END()

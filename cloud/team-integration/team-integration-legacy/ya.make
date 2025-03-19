OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-legacy)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    contrib/java/org/apache/logging/log4j/log4j-api
    contrib/java/yandex/cloud/common/library/data-binding
    contrib/java/yandex/cloud/common/library/repository-core
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/iam-common-exception
    contrib/java/yandex/cloud/iam-common
    contrib/java/yandex/cloud/iam-remote-operation
    contrib/java/yandex/cloud/operations
)

END()

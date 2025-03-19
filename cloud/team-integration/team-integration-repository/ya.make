OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-repository)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    cloud/team-integration/team-integration-legacy

    contrib/java/yandex/cloud/common/library/repository-core
    contrib/java/yandex/cloud/common/library/repository-kikimr
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/common/library/task-processor
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/iam-common
    contrib/java/yandex/cloud/operations
)

END()

RECURSE_FOR_TESTS(
    src/test
    src/testFixtures
)

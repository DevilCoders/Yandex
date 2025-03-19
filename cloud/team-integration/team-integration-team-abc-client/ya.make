OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-team-abc-client)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    cloud/team-integration/team-integration-http-client

    contrib/java/com/fasterxml/jackson/core/jackson-annotations
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/iam-common-exception
    contrib/java/yandex/cloud/iam-common-tvm
)

END()

RECURSE_FOR_TESTS(
    src/test
    src/testFixtures
)

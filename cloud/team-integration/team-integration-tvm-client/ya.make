OWNER(g:cloud-iam)

JAVA_LIBRARY(team-integration-tvm-client)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.main.inc)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    contrib/java/io/prometheus/simpleclient
    contrib/java/yandex/cloud/iam/access-service-client-api
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/iam-common-tvm
)

END()

RECURSE_FOR_TESTS(
    src/testFixtures
)

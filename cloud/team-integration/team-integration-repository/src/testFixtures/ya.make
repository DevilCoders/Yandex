OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.testFixtures.inc)

JAVA_SRCS(SRCDIR java **/*)

PEERDIR(
    cloud/team-integration/team-integration-repository

    contrib/java/yandex/cloud/common/library/repository-core-test
    contrib/java/yandex/cloud/common/library/repository-core
    contrib/java/yandex/cloud/common/library/repository-kikimr
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/iam-common-test
)

END()

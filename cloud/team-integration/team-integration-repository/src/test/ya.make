OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.test.inc)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

PEERDIR(
    cloud/team-integration/team-integration-repository
    cloud/team-integration/team-integration-repository/src/testFixtures

    contrib/java/yandex/cloud/common/library/repository-core
    contrib/java/yandex/cloud/common/library/task-processor
    contrib/java/yandex/cloud/iam-common-test
    contrib/java/yandex/cloud/operations
)

END()

OWNER(g:cloud-iam)

JTEST()

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.test.inc)

JAVA_SRCS(SRCDIR java **/*)
JAVA_SRCS(SRCDIR resources **/*)

PEERDIR(
    cloud/team-integration/team-integration-application

    cloud/team-integration/team-integration-abc/src/testFixtures
    cloud/team-integration/team-integration-cloud-billing-client/src/testFixtures
    cloud/team-integration/team-integration-grpc-server/src/testFixtures
    cloud/team-integration/team-integration-http-server/src/testFixtures
    cloud/team-integration/team-integration-repository/src/testFixtures
    cloud/team-integration/team-integration-team-abc-client/src/testFixtures
    cloud/team-integration/team-integration-team-abcd-client/src/testFixtures

    contrib/java/io/grpc/grpc-api
    contrib/java/javax/inject/javax.inject
    contrib/java/javax/servlet/javax.servlet-api
    contrib/java/yandex/cloud/iam/access-service-client-api
    contrib/java/yandex/cloud/cloud-auth-config
    contrib/java/yandex/cloud/common/library/application
    contrib/java/yandex/cloud/common/library/json-util
    contrib/java/yandex/cloud/common/library/lang
    contrib/java/yandex/cloud/common/library/repository-core
    contrib/java/yandex/cloud/common/library/repository-kikimr
    contrib/java/yandex/cloud/common/library/static-di
    contrib/java/yandex/cloud/common/library/util
    contrib/java/yandex/cloud/fake-cloud-core
    contrib/java/yandex/cloud/iam-common
)

END()

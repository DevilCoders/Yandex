JAVA_LIBRARY(ci-common-ydb)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/common/utils
    ci/common/application-profiles

    ci/clients/http-client-base

    kikimr/public/sdk/java/spring-data-jdbc

    contrib/java/yandex/cloud/common/library/repository-kikimr
    contrib/java/yandex/cloud/common/dependencies/tracing
)

END()


RECURSE_FOR_TESTS(src/test)

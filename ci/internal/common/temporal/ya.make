JAVA_LIBRARY(ci-common-temporal)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/common/utils
    ci/common/application
    ci/common/application-profiles

    ci/clients/logs

    ci/internal/common/ydb

    library/java/annotations

    contrib/java/io/temporal/temporal-sdk
    contrib/java/com/cronutils/cron-utils
    contrib/java/org/apache/logging/log4j/log4j-layout-template-json
)

END()

RECURSE_FOR_TESTS(src/test)

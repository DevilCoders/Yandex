JUNIT5()

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/ci/common/includes/jdk.inc)

JAVA_SRCS(SRCDIR java **/*)

JVM_ARGS(
    --add-opens=java.base/java.text=ALL-UNNAMED
)

PEERDIR(
    market/infra/market-health/config-cs-logshatter/src/test
    market/infra/market-health/logshatter/src/test
)

DATA(
    arcadia/ci/internal/infra/health/logshatter/conf.d
)

EXCLUDE(
    contrib/java/junit/junit
)

END()

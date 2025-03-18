JTEST()

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/ci/common/includes/jdk.inc)

JAVA_SRCS(SRCDIR java **/*)

JVM_ARGS(
    --add-opens=java.base/java.text=ALL-UNNAMED
)

PEERDIR(
    market/infra/market-health/clickphite/src/test
    market/infra/market-health/config-cs-clickphite/src/test
)

DATA(
    arcadia/ci/internal/infra/health/clickphite/conf.d
)


END()

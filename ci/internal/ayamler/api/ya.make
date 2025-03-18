JAVA_PROGRAM(ci-ayamler-api)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)
JAVA_SRCS(SRCDIR src/main/conf **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/common/application
    ci/proto/ayamler

    ci/internal/ci/core
    ci/clients/grpc

    contrib/java/me/dinowernli/java-grpc-prometheus

    #Grpc rate limit interceptors
    contrib/java/com/netflix/concurrency-limits/concurrency-limits-core
    contrib/java/com/netflix/concurrency-limits/concurrency-limits-grpc
    contrib/java/com/netflix/concurrency-limits/concurrency-limits-spectator
    contrib/java/com/netflix/spectator/spectator-reg-micrometer
)

WITH_JDK()
GENERATE_SCRIPT(
    TEMPLATE ${ARCADIA_ROOT}/ci/common/application/src/main/script/application-start.sh
    OUT ${BINDIR}/bin/ci-ayamler-api.sh
    CUSTOM_PROPERTY appName ayamler-api
    CUSTOM_PROPERTY mainClass ru.yandex.ci.ayamler.api.CiAYamlerApiMain
    CUSTOM_PROPERTY log4jConfigurations logs/ci-core.yaml,logs/ci-deploy.yaml
)

END()

RECURSE_FOR_TESTS(src/test)


JAVA_PROGRAM(ci-api)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/conf **/*)

IDEA_RESOURCE_DIRS (src/main/resources/local)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/common/application

    ci/internal/ci/engine

    ci/proto/admin

    contrib/java/me/dinowernli/java-grpc-prometheus
)

WITH_JDK()
GENERATE_SCRIPT(
    TEMPLATE ${ARCADIA_ROOT}/ci/common/application/src/main/script/application-start.sh
    OUT ${BINDIR}/bin/ci-api.sh
    CUSTOM_PROPERTY appName ci-api
    CUSTOM_PROPERTY mainClass ru.yandex.ci.api.CiApiMain
    CUSTOM_PROPERTY log4jConfigurations logs/ci-core.yaml,logs/ci-deploy.yaml
)

END()

RECURSE_FOR_TESTS(
    src/test
)

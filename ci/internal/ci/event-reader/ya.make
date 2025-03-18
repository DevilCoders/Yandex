JAVA_PROGRAM(ci-event-reader)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/conf **/*)

IDEA_RESOURCE_DIRS (src/main/resources/local)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/common/application

    ci/clients/tvm

    ci/internal/ci/engine
)

WITH_JDK()
GENERATE_SCRIPT(
    TEMPLATE ${ARCADIA_ROOT}/ci/common/application/src/main/script/application-start.sh
    OUT ${BINDIR}/bin/ci-event-reader.sh
    CUSTOM_PROPERTY appName ci-event-reader
    CUSTOM_PROPERTY mainClass ru.yandex.ci.event.CiEventReaderMain
    CUSTOM_PROPERTY jvmArgs
        --illegal-access=warn
    CUSTOM_PROPERTY log4jConfigurations logs/ci-core.yaml
)

END()

RECURSE_FOR_TESTS(
    src/test
)

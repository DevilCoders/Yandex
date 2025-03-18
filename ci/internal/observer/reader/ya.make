JAVA_PROGRAM(ci-observer-reader)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/conf **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/internal/observer/core
    ci/common/application
)

WITH_JDK()
GENERATE_SCRIPT(
    TEMPLATE ${ARCADIA_ROOT}/ci/common/application/src/main/script/application-start.sh
    OUT ${BINDIR}/bin/ci-observer-reader.sh
    CUSTOM_PROPERTY appName ci-observer-reader
    CUSTOM_PROPERTY mainClass ru.yandex.ci.observer.reader.CiObserverReaderMain
    CUSTOM_PROPERTY log4jConfigurations logs/ci-core.yaml,logs/ci-logbroker.yaml
)

END()

RECURSE_FOR_TESTS(src/test)


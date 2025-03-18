JAVA_PROGRAM(ci-storage-api)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/conf **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/common/application

    ci/internal/storage/core
)

WITH_JDK()
GENERATE_SCRIPT(
    TEMPLATE ${ARCADIA_ROOT}/ci/common/application/src/main/script/application-start.sh
    OUT ${BINDIR}/bin/ci-storage-api.sh
    CUSTOM_PROPERTY appName ci-storage-api
    CUSTOM_PROPERTY mainClass ru.yandex.ci.storage.api.CiStorageApiMain
    CUSTOM_PROPERTY log4jConfigurations logs/ci-core.yaml,logs/ci-deploy.yaml,logs/ci-logbroker.yaml
)

END()

RECURSE_FOR_TESTS(src/test)


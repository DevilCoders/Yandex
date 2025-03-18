JAVA_PROGRAM(ci-tms)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/conf **/*)

IDEA_RESOURCE_DIRS(src/main/resources/local)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/common/application

    ci/clients/charts
    ci/clients/juggler
    ci/clients/release-machine
    ci/clients/teamcity
    ci/clients/trendbox
    ci/clients/tsum

    ci/internal/ci/engine
)

EXCLUDE(
    contrib/java/asm/asm/3.1
    contrib/java/cglib/cglib-nodep/2.1_3
    contrib/java/aopalliance/aopalliance/1.0
    contrib/java/org/glassfish
    iceberg/inside-conductor
)

# jute.maxbuffer - 32 MiB read buffer for ZK client
WITH_JDK()
GENERATE_SCRIPT(
    TEMPLATE ${ARCADIA_ROOT}/ci/common/application/src/main/script/application-start.sh
    OUT ${BINDIR}/bin/ci-tms.sh
    CUSTOM_PROPERTY appName ci-tms
    CUSTOM_PROPERTY mainClass ru.yandex.ci.tms.CiTmsMain
    CUSTOM_PROPERTY jvmArgs
        --illegal-access=warn
        -Djute.maxbuffer=33554432
    CUSTOM_PROPERTY log4jConfigurations logs/ci-core.yaml,logs/ci-temporal.yaml
)

END()

RECURSE_FOR_TESTS(
    src/test
)

JAVA_PROGRAM(ci-tools)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

DEPENDENCY_MANAGEMENT(
    contrib/java/org/apache/commons/commons-csv/1.8

    contrib/java/org/apache/commons/commons-vfs2/2.8.0
    contrib/java/org/apache/commons/commons-lang3/3.12.0 # vfs2 тащит более свежую версию, чем в зависимостях
    contrib/java/com/squareup/okhttp/okhttp/2.7.5 # vfs2 тащит более свежую версию, чем в зависимостях
)

PEERDIR(
    ci/internal/ci/api
    ci/internal/ci/tms

    ci/internal/storage/tms

    contrib/java/com/beust/jcommander
    contrib/java/org/ehcache/ehcache
    contrib/java/javax/cache/cache-api
    contrib/java/org/apache/commons/commons-vfs2

    contrib/java/org/springframework/boot/spring-boot-starter
    contrib/java/org/springframework/boot/spring-boot-starter-log4j2
    contrib/java/org/springframework/boot/spring-boot-starter-cache

    contrib/java/org/apache/commons/commons-csv

    contrib/java/org/springframework/boot/spring-boot-starter-jdbc
    contrib/java/mysql/mysql-connector-java
)

WITH_JDK()
GENERATE_SCRIPT(
    TEMPLATE ${ARCADIA_ROOT}/ci/common/application/src/main/script/application-start.sh
    OUT ${BINDIR}/bin/ci-tools.sh
    CUSTOM_PROPERTY appName ci-tools
    CUSTOM_PROPERTY mainClass ru.yandex.ci.tools.flows.MigrateS3Logs
    CUSTOM_PROPERTY jvmArgs
        --illegal-access=warn
    CUSTOM_PROPERTY log4jConfigurations logs/ci-core.yaml
)

NO_LINT()

END()

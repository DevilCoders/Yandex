JAVA_LIBRARY(metrics-log4j2)

IF(JDK_VERSION == "")
    JDK_VERSION(11)
ENDIF()
MAVEN_GROUP_ID(ru.yandex.monlib)

OWNER(g:solomon)

JAVA_SRCS(
    PACKAGE_PREFIX ru.yandex.monlib.metrics.log4j2
    SRCDIR src
    **/*.java
)

PEERDIR(
    library/java/monlib/metrics
    contrib/java/org/apache/logging/log4j/log4j-api/2.11.0
    contrib/java/org/apache/logging/log4j/log4j-core/2.11.0
)

LINT(base)
END()

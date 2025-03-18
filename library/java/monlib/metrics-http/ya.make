JAVA_LIBRARY(metrics-http)

IF(JDK_VERSION == "")
    JDK_VERSION(11)
ENDIF()
MAVEN_GROUP_ID(ru.yandex.monlib)

OWNER(g:solomon)

JAVA_SRCS(
    PACKAGE_PREFIX ru.yandex.monlib.metrics.http
    SRCDIR src
    **/*.java
)

PEERDIR(
    library/java/monlib/metrics
)

LINT(base)
END()

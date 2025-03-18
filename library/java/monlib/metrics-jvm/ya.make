JAVA_LIBRARY(metrics-jvm)

IF(JDK_VERSION == "")
    JDK_VERSION(11)
ENDIF()
MAVEN_GROUP_ID(ru.yandex.monlib)

OWNER(g:solomon)

JAVA_SRCS(
    PACKAGE_PREFIX ru.yandex.monlib.metrics
    SRCDIR src
    **/*.java
)

PEERDIR(
    library/java/monlib/metrics

    contrib/java/com/google/code/findbugs/jsr305/3.0.2
)

LINT(base)
END()


JAVA_LIBRARY(metrics-log4j)

JDK_VERSION(11)
MAVEN_GROUP_ID(ru.yandex.monlib)

OWNER(g:solomon)

JAVA_SRCS(
    PACKAGE_PREFIX ru.yandex.monlib.metrics
    SRCDIR src
    **/*.java
)

PEERDIR(
    library/java/monlib/metrics
    contrib/java/log4j/log4j/1.2.17
)

LINT(base)
END()

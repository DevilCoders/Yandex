JAVA_LIBRARY(metrics)

IF(JDK_VERSION == "")
    JDK_VERSION(11)
ENDIF()
MAVEN_GROUP_ID(ru.yandex.monlib)

OWNER(g:solomon)

ENABLE(SOURCES_JAR)

JAVA_SRCS(
    PACKAGE_PREFIX ru.yandex.monlib.metrics
    SRCDIR src
    **/*.java
)

PEERDIR(
    contrib/java/com/google/code/findbugs/jsr305/3.0.2
    contrib/java/org/slf4j/slf4j-api/1.7.25
    contrib/java/org/lz4/lz4-java/1.4.1
    contrib/java/com/github/luben/zstd-jni/1.3.2-4
)

LINT(base)
END()

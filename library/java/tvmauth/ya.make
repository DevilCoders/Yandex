JAVA_LIBRARY(tvmauth-java)

IF(JDK_VERSION == "")
    JDK_VERSION(11)
ENDIF()

MAVEN_GROUP_ID(ru.yandex.passport)

ADD_DLLS_TO_JAR()

ENABLE(SOURCES_JAR)

OWNER(g:passport_infra)

PEERDIR(
    contrib/java/com/google/code/findbugs/jsr305
    contrib/java/com/google/code/gson/gson
    contrib/java/org/slf4j/slf4j-api
    library/java/tvmauth/src/c
)

DEPENDENCY_MANAGEMENT(
    contrib/java/com/google/code/findbugs/jsr305/3.0.2
    contrib/java/com/google/code/gson/gson/2.8.6
    contrib/java/org/slf4j/slf4j-api/1.7.0
)

JAVA_SRCS(
    SRCDIR
    src/main/java
    **/*
)

LINT(extended)

END()

IF (NOT MUSL AND NOT OS_WINDOWS AND NOT SANITIZER_TYPE)
    RECURSE_FOR_TESTS(
        src/ut
    )
ENDIF()

RECURSE_FOR_TESTS(
    build_jar_with_so/packager
    examples
)

RECURSE(
    dynamic_dst
)

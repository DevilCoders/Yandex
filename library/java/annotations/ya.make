JAVA_LIBRARY(yandex-annotations)

IF(JDK_VERSION == "")
    JDK_VERSION(11)
ENDIF()
MAVEN_GROUP_ID(ru.yandex)

OWNER(andreevdm)

ENABLE(SOURCES_JAR)

USE_ERROR_PRONE()
LINT(extended)
CHECK_JAVA_DEPS(yes)

JAVA_SRCS(SRCDIR src/main/java **/*)

PEERDIR(
    contrib/java/com/google/code/findbugs/jsr305/3.0.2
)

END()

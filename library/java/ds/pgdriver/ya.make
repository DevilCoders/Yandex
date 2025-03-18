JAVA_LIBRARY()

IF(JDK_VERSION == "")
    JDK_VERSION(11)
ENDIF()

PEERDIR(
    library/java/annotations

    contrib/java/org/postgresql/postgresql/42.2.24
)

USE_ERROR_PRONE()
LINT(extended)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

END()

RECURSE_FOR_TESTS(src/test)

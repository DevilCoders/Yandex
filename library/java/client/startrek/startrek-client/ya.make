JAVA_LIBRARY()

JDK_VERSION(11)

OWNER(g:mobdevtools)

WITH_KOTLIN()
JAVA_SRCS(SRCDIR src/main/kotlin **/*)

INCLUDE(${ARCADIA_ROOT}/library/java/client/ya.dependency_management.inc)
PEERDIR(
    library/java/client/common
)

END()

RECURSE_FOR_TESTS(src/test)

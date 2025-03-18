JUNIT5()

JDK_VERSION(11)

OWNER(g:mobdevtools)

WITH_KOTLIN()
JAVA_SRCS(SRCDIR kotlin **/*)

INCLUDE(${ARCADIA_ROOT}/library/java/client/ya.dependency_management.inc)
PEERDIR(
    library/java/client/startrek/startrek-fake
)

END()

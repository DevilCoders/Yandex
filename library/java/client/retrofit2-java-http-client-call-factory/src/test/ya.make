JUNIT5()

JDK_VERSION(11)

OWNER(g:mobdevtools)

WITH_KOTLIN()
JAVA_SRCS(SRCDIR kotlin **/*)

INCLUDE(${ARCADIA_ROOT}/library/java/client/ya.dependency_management.inc)
PEERDIR(
    library/java/client/retrofit2-java-http-client-call-factory

    contrib/java/com/squareup/okhttp3/mockwebserver
    contrib/java/com/squareup/retrofit2/converter-jackson
    contrib/java/com/squareup/retrofit2/retrofit-mock
    contrib/java/org/junit/vintage/junit-vintage-engine
)

END()

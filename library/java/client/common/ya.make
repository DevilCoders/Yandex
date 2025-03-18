JAVA_LIBRARY()

JDK_VERSION(11)

OWNER(g:mobdevtools)

WITH_KOTLIN()
JAVA_SRCS(SRCDIR src/main/kotlin **/*)

INCLUDE(${ARCADIA_ROOT}/library/java/client/ya.dependency_management.inc)
PEERDIR(
    contrib/java/com/fasterxml/jackson/module/jackson-module-kotlin
    contrib/java/com/squareup/okhttp3/okhttp
    contrib/java/com/squareup/retrofit2/converter-jackson
    contrib/java/com/squareup/retrofit2/retrofit
)

END()


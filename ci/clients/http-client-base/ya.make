JAVA_LIBRARY(ci-http-client-base)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    library/java/annotations

    contrib/java/com/google/guava/guava
    contrib/java/one/util/streamex
    contrib/java/org/slf4j/slf4j-api

    contrib/java/jakarta/servlet/jakarta.servlet-api

    contrib/java/com/fasterxml/jackson/datatype/jackson-datatype-jsr310
    contrib/java/com/fasterxml/jackson/datatype/jackson-datatype-jdk8

    contrib/java/com/squareup/retrofit2/retrofit
    contrib/java/com/squareup/retrofit2/converter-jackson
    contrib/java/org/asynchttpclient/async-http-client-extras-retrofit2
)

END()

RECURSE_FOR_TESTS(src/test)

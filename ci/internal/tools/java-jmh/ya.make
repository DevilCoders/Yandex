JUNIT5()

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    contrib/java/com/google/guava/guava
    contrib/java/org/openjdk/jmh/jmh-core
    contrib/java/org/openjdk/jmh/jmh-generator-annprocess
)

END()

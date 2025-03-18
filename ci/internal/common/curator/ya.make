JAVA_LIBRARY(ci-common-curator)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    contrib/java/org/apache/curator/curator-recipes
)

END()


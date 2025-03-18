JAVA_LIBRARY(tracker-client)

JAVA_SRCS(SRCDIR src/main/java **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    tracker/tracker-java-client/tracker-client/src/main
)

END()

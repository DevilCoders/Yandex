JAVA_LIBRARY(ci-observer-core)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/internal/ci/core
    ci/internal/storage/core

    ci/proto/observer
)

END()

RECURSE_FOR_TESTS(src/test)


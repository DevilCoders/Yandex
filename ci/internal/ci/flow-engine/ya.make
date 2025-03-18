JAVA_LIBRARY(ci-flow-engine)

JAVA_SRCS(SRCDIR src/main/java **/*)
JAVA_SRCS(SRCDIR src/main/resources **/*)

INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR(
    ci/internal/ci/core

    ci/internal/common/bazinga
    ci/internal/common/curator

    contrib/java/jakarta/annotation/jakarta.annotation-api
    contrib/java/jakarta/validation/jakarta.validation-api

    contrib/java/io/reactivex/rxjava2/rxjava

    contrib/java/org/reflections/reflections
)

END()

RECURSE_FOR_TESTS(src/test)


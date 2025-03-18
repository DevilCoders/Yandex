INCLUDE(${ARCADIA_ROOT}/ci/common/ci-includes.inc)

PEERDIR (
    ci/common/utils

    contrib/java/org/junit/jupiter/junit-jupiter

    contrib/java/org/mockito/mockito-core
    contrib/java/org/mockito/mockito-junit-jupiter

    contrib/java/org/assertj/assertj-core

    contrib/java/org/mock-server/mockserver-junit-jupiter
    contrib/java/org/mock-server/mockserver-netty
)

EXCLUDE(
    contrib/java/org/hamcrest/hamcrest-core
    contrib/java/org/hamcrest/hamcrest-library/1.3
)

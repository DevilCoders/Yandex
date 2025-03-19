SIZE(SMALL)

INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.common.inc)

PEERDIR(
    contrib/java/junit/junit
    contrib/java/org/assertj/assertj-core
    contrib/java/org/mockito/mockito-core
)

CHECK_DEPENDENT_DIRS(DENY PEERDIRS
    GLOB cloud/team-integration/*/src/test
)

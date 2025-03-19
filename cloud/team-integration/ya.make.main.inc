INCLUDE(${ARCADIA_ROOT}/cloud/team-integration/ya.make.common.inc)

CHECK_DEPENDENT_DIRS(DENY PEERDIRS
    GLOB cloud/team-integration/*/src/test
    GLOB cloud/team-integration/*/src/testFixtures
)

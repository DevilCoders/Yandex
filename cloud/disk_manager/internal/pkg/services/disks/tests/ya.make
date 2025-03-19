OWNER(g:cloud-nbs)

GO_TEST_FOR(cloud/disk_manager/internal/pkg/services/disks)

SET(RECIPE_ARGS nbs)
INCLUDE(${ARCADIA_ROOT}/cloud/disk_manager/test/recipe/recipe.inc)

GO_XTEST_SRCS(
    disks_test.go
)

SIZE(LARGE)
TAG(ya:fat ya:force_sandbox ya:sandbox_coverage)

REQUIREMENTS(
    cpu:4
    ram:16
)

END()

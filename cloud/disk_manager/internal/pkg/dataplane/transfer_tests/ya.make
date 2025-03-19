OWNER(g:cloud-nbs)

GO_TEST_FOR(cloud/disk_manager/internal/pkg/dataplane)

SET(RECIPE_ARGS nbs)
INCLUDE(${ARCADIA_ROOT}/cloud/disk_manager/test/recipe/recipe.inc)

GO_XTEST_SRCS(
    transfer_test.go
)

SIZE(MEDIUM)
TAG(sb:ssd)

REQUIREMENTS(
    ram:32
)

END()

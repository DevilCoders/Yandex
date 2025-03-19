OWNER(g:cloud-nbs)

GO_TEST_FOR(cloud/disk_manager/internal/pkg/dataplane/nbs)

SET(RECIPE_ARGS nbs)
INCLUDE(${ARCADIA_ROOT}/cloud/disk_manager/test/recipe/recipe.inc)

GO_XTEST_SRCS(
    nbs_test.go
)

SIZE(MEDIUM)

END()

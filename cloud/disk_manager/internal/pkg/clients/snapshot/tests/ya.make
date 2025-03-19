OWNER(g:cloud-nbs)

GO_TEST_FOR(cloud/disk_manager/internal/pkg/clients/snapshot)

INCLUDE(${ARCADIA_ROOT}/cloud/compute/snapshot/testrecipe/recipe.inc)
FROM_SANDBOX(
    FILE
    1406454693
    OUT_NOAUTO
    snapshot-qemu-nbd-docker-image.tar
)
DATA(sbr://1406454693)

SET(RECIPE_ARGS snapshot)
INCLUDE(${ARCADIA_ROOT}/cloud/disk_manager/test/recipe/recipe.inc)

GO_XTEST_SRCS(
    client_test.go
)

SIZE(LARGE)
TAG(ya:fat ya:force_sandbox ya:sandbox_coverage)

REQUIREMENTS(
    cpu:4
    ram:16
    container:773239891
)

PEERDIR(
    cloud/disk_manager/internal/pkg/clients/nbs
)

END()

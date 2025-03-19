OWNER(g:cloud-nbs)

INCLUDE(${ARCADIA_ROOT}/cloud/compute/snapshot/testrecipe/recipe.inc)
FROM_SANDBOX(
    FILE
    1406454693
    OUT_NOAUTO
    snapshot-qemu-nbd-docker-image.tar
)
DATA(sbr://1406454693)

SET_APPEND(RECIPE_ARGS multiple-disk-managers)
INCLUDE(${ARCADIA_ROOT}/cloud/disk_manager/test/recipe/recipe.inc)

FORK_SUBTESTS()
SPLIT_FACTOR(4)

SIZE(LARGE)
TAG(ya:fat ya:force_sandbox ya:sandbox_coverage sb:ssd)

REQUIREMENTS(
    cpu:4
    ram:16
    disk_usage:200
    container:773239891
)

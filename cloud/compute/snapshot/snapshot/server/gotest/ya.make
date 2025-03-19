GO_TEST_FOR(cloud/compute/snapshot/snapshot/server)

OWNER(g:cloud-nbs)

INCLUDE(${ARCADIA_ROOT}/cloud/compute/snapshot/testrecipe/recipe.inc)

FROM_SANDBOX(
    FILE
    1406454693
    OUT_NOAUTO
    snapshot-qemu-nbd-docker-image.tar
)

SIZE(LARGE)

TAG(
    ya:fat
    ya:force_sandbox
    ya:nofuse
)

DATA(sbr://1406454693)

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

DATA(arcadia/cloud/compute/snapshot/config.toml)

REQUIREMENTS(
    container:773239891
    # xenial
    cpu:all
    dns:dns64
)

END()

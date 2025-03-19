GO_TEST_FOR(cloud/compute/snapshot/internal/dockerprocess)

OWNER(g:cloud-nbs)

INCLUDE(${ARCADIA_ROOT}/cloud/compute/snapshot/testrecipe/recipe.inc)

FROM_SANDBOX(
    FILE
    1406454693
    OUT_NOAUTO
    snapshot-qemu-nbd-docker-image.tar
)

DATA(sbr://1406454693)

SIZE(LARGE)

TAG(
    ya:fat
    ya:force_sandbox
    ya:nofuse
)

REQUIREMENTS(
    container:773239891
    # xenial
    cpu:all
    dns:dns64
)

END()

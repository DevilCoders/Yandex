GO_TEST_FOR(cloud/compute/snapshot/snapshot/lib/nbd)

OWNER(g:cloud-nbs)

SET(
    DOCKER_CONTEXT_FILE
    cloud/compute/snapshot/docker/qemu/docker-context.yml
)

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

FROM_SANDBOX(
    FILE
    1358547184
    OUT_NOAUTO
    cirros-0.3.5-x86_64-disk.img
)

DATA(sbr://1358547184)

END()

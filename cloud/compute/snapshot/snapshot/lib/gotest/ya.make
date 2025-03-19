GO_TEST_FOR(cloud/compute/snapshot/snapshot/lib)

OWNER(g:cloud-nbs)

DATA(arcadia/cloud/compute/snapshot/config.toml)

ENV(YDB_DISABLE_LEGACY_YQL=false)
ENV(YDB_YQL_SYNTAX_VERSION="0")
ENV(TEST_CONFIG_PATH=cloud/compute/snapshot/config.toml)

INCLUDE(${ARCADIA_ROOT}/cloud/compute/snapshot/testrecipe/recipe.inc)

FROM_SANDBOX(
    FILE
    1406454693
    OUT_NOAUTO
    snapshot-qemu-nbd-docker-image.tar
)

DATA(sbr://1406454693)


INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

INCLUDE(${ARCADIA_ROOT}/cloud/blockstore/tests/recipes/local-kikimr/local-kikimr-stable.inc)

FROM_SANDBOX(
    1358547184
    OUT_NOAUTO
    cirros-0.3.5-x86_64-disk.img
)

DATA(sbr://1358547184)

FROM_SANDBOX(
    1398323033
    OUT_NOAUTO
    random-image.raw
)

DATA(sbr://1398323033)

SIZE(LARGE)

TAG(
    ya:fat
    ya:force_sandbox
    ya:noretries
)

REQUIREMENTS(
    container:773239891
    # xenial
    cpu:all
    dns:dns64
)

END()

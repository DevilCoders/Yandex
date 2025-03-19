GO_TEST_FOR(cloud/compute/snapshot/cmd/yc-snapshot-download)

OWNER(g:cloud-nbs)

ENV(YDB_DISABLE_LEGACY_YQL=false)

INCLUDE(${ARCADIA_ROOT}/cloud/compute/snapshot/testrecipe/recipe.inc)

FROM_SANDBOX(
    FILE
    1406454693
    OUT_NOAUTO
    snapshot-qemu-nbd-docker-image.tar
)

DATA(sbr://1406454693)

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

END()

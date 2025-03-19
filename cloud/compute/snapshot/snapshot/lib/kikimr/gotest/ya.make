GO_TEST_FOR(cloud/compute/snapshot/snapshot/lib/kikimr)

OWNER(g:cloud-nbs)

ENV(YDB_DISABLE_LEGACY_YQL=false)

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

DATA(arcadia/cloud/compute/snapshot/config.toml)

END()

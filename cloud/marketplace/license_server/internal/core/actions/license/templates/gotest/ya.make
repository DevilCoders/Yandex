GO_TEST_FOR(cloud/marketplace/license_server/internal/core/actions/license/templates)

OWNER(g:cloud-marketplace)

ENV(YDB_USE_IN_MEMORY_PDISKS=true)

ENV(YDB_TYPE=LOCAL)

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

TIMEOUT(590)

SIZE(MEDIUM)

END()

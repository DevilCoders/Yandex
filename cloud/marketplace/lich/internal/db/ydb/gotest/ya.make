GO_TEST_FOR(cloud/marketplace/lich/internal/db/ydb)

OWNER(g:cloud-marketplace)

ENV(YDB_USE_IN_MEMORY_PDISKS=true)
ENV(YDB_TYPE=LOCAL)

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)
DATA(arcadia/cloud/marketplace/lich/internal/db/ydb/gotest/sql)

TIMEOUT(590)

SIZE(MEDIUM)

END()

GO_TEST_FOR(cloud/billing/go/piper/internal/db/ydb/reports)

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

TIMEOUT(590)

SIZE(MEDIUM)

DATA(arcadia/cloud/billing/go/piper/internal/db/ydb/)

END()

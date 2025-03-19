GO_TEST_FOR(cloud/bootstrap/ydb-lock/test/e2e)

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

SIZE(MEDIUM)

TIMEOUT(360)

END()

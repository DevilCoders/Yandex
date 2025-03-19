GO_TEST_FOR(cloud/mdb/mdb-health/pkg/healthstore/clickhouse)

OWNER(g:mdb)

SET(
    CLICKHOUSE_VERSION
    "21.3.7.62"
)

SET(
    CLICKHOUSE_RECIPE_OPTS
    --execute
    cloud/mdb/mdb-health/pkg/healthstore/clickhouse/gotest/test_tables.sql
)

INCLUDE(${ARCADIA_ROOT}/library/recipes/clickhouse/recipe.inc)

END()

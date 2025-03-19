GO_TEST()

OWNER(g:mdb)

IF (OS_LINUX)
    SET(
        CLICKHOUSE_VERSION
        "21.3.7.62"
    )

    INCLUDE(${ARCADIA_ROOT}/library/recipes/zookeeper/recipe.inc)

    INCLUDE(${ARCADIA_ROOT}/library/recipes/clickhouse/recipe.inc)
ENDIF()

SIZE(MEDIUM)

GO_TEST_SRCS(clickhouse_test.go)

GO_TEST_EMBED_PATTERN(schema/datacloud.sql)

END()

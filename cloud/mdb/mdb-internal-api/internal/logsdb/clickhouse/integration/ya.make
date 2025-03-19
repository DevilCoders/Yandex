GO_TEST()

OWNER(g:mdb)

IF (OS_LINUX)
    SET(
        CLICKHOUSE_VERSION
        "20.3.9.70"
    )

    INCLUDE(${ARCADIA_ROOT}/library/recipes/clickhouse/recipe.inc)
ENDIF()

SIZE(MEDIUM)

GO_TEST_SRCS(clickhouse_test.go)

END()

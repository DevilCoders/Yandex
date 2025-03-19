GO_LIBRARY()

OWNER(
    g:data-transfer
    tserakhau
)

ENV(
    LOG_LEVEL=debug
    CI=1
)

SIZE(MEDIUM)

DATA(arcadia/cloud/dataplatform/connman/internal/storage/migrations)

USE_RECIPE(
    antiadblock/postgres_local/recipe/recipe
    --user
    postgres
    --db_name
    postgres
    --migrations_path
    cloud/dataplatform/connman/internal/storage/migrations
)

INCLUDE(${ARCADIA_ROOT}/antiadblock/postgres_local/recipe/postgresql_bin.inc)

DEPENDS(antiadblock/postgres_local/recipe)

SRCS(
    mocks.go
    recipe.go
)

GO_TEST_SRCS(basic_test.go)

END()

RECURSE(gotest)

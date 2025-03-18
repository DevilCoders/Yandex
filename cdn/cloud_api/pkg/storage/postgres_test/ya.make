GO_TEST()

OWNER(
    g:strm-admin
    g:traffic
)

USE_RECIPE(
    antiadblock/postgres_local/recipe/recipe
    --migrations_path
    cdn/cloud_api/migrations
    --port
    5432
    --user
    postgres
    --password
    postgres
    --db_name
    postgres
    --use-text
)

DATA(arcadia/cdn/cloud_api/migrations)

INCLUDE(${ARCADIA_ROOT}/antiadblock/postgres_local/recipe/postgresql_bin.inc)

DEPENDS(antiadblock/postgres_local/recipe)

SIZE(MEDIUM)

REQUIREMENTS(network:full)

GO_XTEST_SRCS(
    admin_test.go
    db_test.go
    origin_test.go
    origins_group_test.go
    resource_rule_test.go
    resource_test.go
    secondary_hostnames_test.go
)

END()

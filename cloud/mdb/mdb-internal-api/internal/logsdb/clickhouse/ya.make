GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clickhouse.go
    helpers.go
    queries.go
)

END()

RECURSE(
    integration
    sqlmock
)

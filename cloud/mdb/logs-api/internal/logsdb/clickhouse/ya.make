GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clickhouse.go
    config.go
    helpers.go
    queries.go
)

END()

RECURSE(integration)

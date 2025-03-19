GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    chmodels.go
    clickhouse.go
    queries.go
)

END()

RECURSE(integration)

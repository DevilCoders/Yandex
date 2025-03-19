GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clickhouse.go
    cluster.go
    dictionaries.go
    settings.go
    shard.go
    users.go
    zookeeper.go
)

GO_TEST_SRCS(clickhouse_test.go)

END()

RECURSE(gotest)

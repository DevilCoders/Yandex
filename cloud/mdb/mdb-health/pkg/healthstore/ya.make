GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    async_store.go
    config.go
    healthstore.go
    stats.go
)

GO_XTEST_SRCS(async_store_test.go)

END()

RECURSE(
    clickhouse
    gotest
    mocks
)

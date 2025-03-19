GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clickhouse.go
    tests.go
)

GO_TEST_SRCS(clickhouse_test.go)

END()

RECURSE(gotest)

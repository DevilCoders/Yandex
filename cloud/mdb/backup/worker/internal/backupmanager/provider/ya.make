GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    clickhouse.go
    common.go
    mongodb.go
    mysql.go
    postgresql.go
)

GO_TEST_SRCS(
    common_test.go
    postgresql_test.go
)

END()

RECURSE(gotest)

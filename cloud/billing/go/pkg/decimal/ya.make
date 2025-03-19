GO_LIBRARY()

OWNER(g:cloud-billing)

SRCS(
    arithmetic.go
    byteop.go
    clickhouse.go
    constants.go
    converters.go
    decimal.go
    doc.go
    encoding.go
    errors.go
    parse.go
    pools.go
    scale.go
    sql.go
    string.go
    ydb.go
)

GO_TEST_SRCS(
    arithmetic_test.go
    bench_test.go
    clickhouse_test.go
    converters_test.go
    decimal_test.go
    encoding_test.go
    parse_test.go
    sql_test.go
    string_test.go
    ydb_test.go
)

END()

RECURSE(gotest)

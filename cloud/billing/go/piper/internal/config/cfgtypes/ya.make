GO_LIBRARY()

SRCS(
    auth.go
    dump.go
    clickhouse.go
    enums.go
    enums_string.go
    interconnect.go
    lockbox.go
    pkg_meta.go
    logbroker.go
    resharder.go
    service_parts.go
    status.go
    tls.go
    tooling.go
    types.go
    ydb.go
)

GO_TEST_SRCS(types_test.go)

END()

RECURSE(gotest)

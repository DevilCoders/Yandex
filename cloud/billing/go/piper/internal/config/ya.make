GO_LIBRARY()

SRCS(
    auth.go
    clickhouse.go
    cloud_config.go
    configuration.go
    configuration_override.go
    di.go
    dumper.go
    interconnect.go
    logbroker.go
    resharder.go
    run_control.go
    services.go
    status.go
    tls.go
    tooling.go
    types.go
    ydb.go
)

GO_TEST_SRCS(
    common_test.go
    configuration_test.go
)

END()

RECURSE(
    cfgtypes
    gotest
    loader
    states
)

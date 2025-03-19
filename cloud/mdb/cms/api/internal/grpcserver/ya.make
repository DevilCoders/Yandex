GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    app.go
    auth.go
    common.go
    conductor.go
    config.go
    duty.go
    get_operation.go
    health.go
    list_operations.go
    move_instance.go
    operation.go
    resolve_instances.go
    server.go
    whip_primary.go
)

GO_TEST_SRCS(duty_test.go)

GO_XTEST_SRCS(
    conductor_test.go
    move_instance_test.go
    server_test.go
)

END()

RECURSE_FOR_TESTS(gotest)

GO_TEST()

OWNER(g:mdb-dataproc)

GO_TEST_SRCS(
    integration_test.go
    knoxless_test.go
    websocket_test.go
)

END()

RECURSE(
    agent
    common
    server
    testutil
)

GO_LIBRARY()

OWNER(g:mdb-dataproc)

SRCS(
    app.go
    handlers.go
    health.go
    ping_handler.go
    server.go
)

GO_TEST_SRCS(
    handlers_test.go
    health_test.go
)

END()

RECURSE(gotest)

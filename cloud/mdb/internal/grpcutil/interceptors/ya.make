GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    attributes.go
    auth.go
    chain.go
    errors.go
    idempotence.go
    logging.go
    panic.go
    readonly.go
    requestid.go
    restricted.go
    retry.go
    sentry.go
    server_stream.go
    tracing.go
)

GO_TEST_SRCS(
    idempotence_test.go
    restricted_test.go
    retry_test.go
    sentry_test.go
    tracing_test.go
)

END()

RECURSE(gotest)

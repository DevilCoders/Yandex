GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    attributes.go
    bodysigning.go
    errors.go
    http.go
    instrumentation.go
    middleware.go
    request.go
    requestid.go
    sentry.go
    transport.go
)

GO_TEST_SRCS(sentry_test.go)

GO_XTEST_SRCS(transport_test.go)

END()

RECURSE(
    gotest
    openapi
)

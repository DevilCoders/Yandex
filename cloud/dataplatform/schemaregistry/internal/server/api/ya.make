GO_LIBRARY()

OWNER(tserakhau)

SRCS(
    api.go
    authorizer.go
    namespace.go
    ping.go
    schema.go
    search.go
    version.go
)

GO_XTEST_SRCS(
    api_test.go
    schema_test.go
)

END()

RECURSE(gotest)

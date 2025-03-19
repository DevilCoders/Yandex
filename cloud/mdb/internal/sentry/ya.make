GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    client.go
    errors.go
    noop.go
)

GO_XTEST_SRCS(client_test.go)

END()

RECURSE(
    gotest
    mocks
    raven
    tags
)

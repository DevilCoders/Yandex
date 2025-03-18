GO_LIBRARY()

OWNER(
    buglloc
    g:go-library
)

SRCS(
    aliases.go
    attributes.go
    client.go
    doc.go
    errors.go
    requests.go
    responses.go
    statuses.go
)

GO_XTEST_SRCS(errors_test.go)

END()

RECURSE(
    example-app
    gotest
    httpbb
    mocks
)

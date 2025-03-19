GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    dynamic.go
    mock.go
    server.go
)

GO_XTEST_SRCS(server_test.go)

END()

RECURSE(
    expectations
    gotest
    parsers
    testproto
)

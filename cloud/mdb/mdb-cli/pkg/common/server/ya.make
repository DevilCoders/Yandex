GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    doc.go
    server.go
)

GO_TEST_SRCS(server_test.go)

END()

RECURSE(gotest)

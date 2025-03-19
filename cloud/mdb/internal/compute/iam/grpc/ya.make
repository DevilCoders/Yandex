GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    abc.go
    credentials.go
    grpc.go
)

GO_TEST_SRCS(credentials_test.go)

END()

RECURSE(gotest)

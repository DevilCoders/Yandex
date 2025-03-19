GO_LIBRARY()

OWNER(g:cloud-marketplace)

SRCS(
    client.go
    errors.go
)

GO_TEST_SRCS(client_test.go)

END()

RECURSE(gotest)

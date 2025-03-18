GO_LIBRARY()

OWNER(
    roboslone
    g:go-library
)

SRCS(
    client.go
    options.go
    storage.go
)

GO_TEST_SRCS(
    client_test.go
    webauthmock_test.go
)

END()

RECURSE_FOR_TESTS(gotest)

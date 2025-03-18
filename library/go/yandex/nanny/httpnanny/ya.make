GO_LIBRARY()

OWNER(
    prime
    g:go-library
)

SRCS(
    http.go
    options.go
)

GO_TEST_SRCS(
    client_test.go
    example_test.go
    helper_test.go
)

END()

RECURSE(gotest)

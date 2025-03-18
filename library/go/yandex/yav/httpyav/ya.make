GO_LIBRARY()

OWNER(
    gzuykov
    g:go-library
    g:passport_python
)

SRCS(
    client.go
    client_opts.go
    request.go
    rsa.go
)

GO_TEST_SRCS(
    client_opts_test.go
    client_test.go
    request_test.go
    rsa_test.go
)

GO_XTEST_SRCS(
    client_mock_test.go
    example_client_test.go
)

END()

RECURSE(gotest)

GO_LIBRARY()

OWNER(
    gzuykov
    g:go-library
)

SRCS(
    client.go
    client_opts.go
)

GO_TEST_SRCS(
    client_opts_test.go
    client_test.go
    golden_test.go
)

GO_XTEST_SRCS(client_mock_test.go)

END()

RECURSE(gotest)

GO_LIBRARY()

OWNER(
    gzuykov
    g:go-library
    g:passport_python
)

SRCS(
    client.go
    errors.go
    request_secret.go
    request_token.go
    request_version.go
    response.go
    types.go
)

GO_TEST_SRCS(
    response_test.go
    types_test.go
)

END()

RECURSE(
    gotest
    httpyav
)

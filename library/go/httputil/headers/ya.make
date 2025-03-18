GO_LIBRARY()

OWNER(
    g:go-library
    gzuykov
)

SRCS(
    accept.go
    authorization.go
    content.go
    cookie.go
    user_agent.go
    warning.go
)

GO_TEST_SRCS(warning_test.go)

GO_XTEST_SRCS(
    accept_test.go
    authorization_test.go
    content_test.go
)

END()

RECURSE(gotest)

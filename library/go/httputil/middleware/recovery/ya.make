GO_LIBRARY()

OWNER(g:go-library)

SRCS(
    middleware.go
    middleware_opts.go
)

GO_TEST_SRCS(
    middleware_opts_test.go
    middleware_test.go
)

END()

RECURSE(gotest)

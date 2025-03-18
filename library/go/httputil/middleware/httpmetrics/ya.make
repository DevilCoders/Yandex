GO_LIBRARY()

OWNER(g:go-library)

SRCS(
    middleware.go
    middleware_opts.go
)

GO_TEST_SRCS(middleware_test.go)

GO_XTEST_SRCS(example_test.go)

END()

RECURSE(gotest)

GO_LIBRARY()

OWNER(
    g:go-library
    floatdrop
    dimastark
)

SRCS(
    generator.go
    middleware.go
    middleware_opts.go
)

GO_TEST_SRCS(middleware_test.go)

END()

RECURSE(gotest)

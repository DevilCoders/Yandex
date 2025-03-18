GO_LIBRARY()

OWNER(
    g:go-library
    g:solomon
)

SRCS(
    errors.go
    pusher.go
    pusher_opts.go
)

GO_TEST_SRCS(
    pusher_opts_test.go
    pusher_test.go
)

GO_XTEST_SRCS(example_test.go)

END()

RECURSE(gotest)

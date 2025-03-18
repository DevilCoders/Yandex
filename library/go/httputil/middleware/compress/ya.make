GO_LIBRARY()

OWNER(g:go-library)

SRCS(
    gzip.go
    handler.go
)

GO_TEST_SRCS(
    compress_test.go
    gzip_test.go
    handler_test.go
)

GO_XTEST_SRCS(example_test.go)

END()

RECURSE(gotest)

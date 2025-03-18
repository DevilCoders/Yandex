GO_LIBRARY()

OWNER(
    g:kikimr
    g:go-library
)

SRCS(
    iam.go
    metadata.go
)

GO_TEST_SRCS(
    iam_test.go
)

END()

RECURSE_FOR_TESTS(gotest)

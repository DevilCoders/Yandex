GO_LIBRARY()

OWNER(
    g:go-library
    gzuykov
)

SRCS(scopelint.go)

GO_TEST_SRCS(scopelint_test.go)

END()

RECURSE(gotest)

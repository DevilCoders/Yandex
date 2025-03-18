GO_LIBRARY()

OWNER(
    g:go-library
    gzuykov
)

SRCS(errcheck.go)

GO_XTEST_SRCS(errcheck_test.go)

END()

RECURSE(gotest)

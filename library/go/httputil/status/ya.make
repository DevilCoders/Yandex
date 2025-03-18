GO_LIBRARY()

OWNER(
    gzuykov
    g:go-library
)

SRCS(status.go)

GO_XTEST_SRCS(status_test.go)

END()

RECURSE(gotest)

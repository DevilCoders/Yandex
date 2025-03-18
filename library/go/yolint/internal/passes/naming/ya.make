GO_LIBRARY()

OWNER(
    gzuykov
    g:go-library
)

SRCS(naming.go)

GO_XTEST_SRCS(naming_test.go)

END()

RECURSE(gotest)

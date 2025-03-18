GO_LIBRARY()

OWNER(
    g:go-library
    gzuykov
)

SRCS(alias.go)

GO_XTEST_SRCS(alias_test.go)

END()

RECURSE(gotest)

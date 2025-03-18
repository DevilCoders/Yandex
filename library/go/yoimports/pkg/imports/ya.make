GO_LIBRARY()

OWNER(
    gzuykov
    buglloc
    g:go-library
)

SRCS(
    common.go
    formatter.go
    imports.go
    opts.go
)

GO_TEST_SRCS(
    common_test.go
    formatter_test.go
)

END()

RECURSE(
    gotest
    resolver
)

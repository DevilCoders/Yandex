GO_LIBRARY()

OWNER(
    gzuykov
    buglloc
    g:go-library
)

SRCS(
    common.go
    gomod.go
    gopath.go
    opts.go
    resolver.go
)

GO_XTEST_SRCS(resolver_test.go)

END()

RECURSE(gotest)

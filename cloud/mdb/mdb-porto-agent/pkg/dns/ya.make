GO_LIBRARY()

OWNER(g:mdb)

SRCS(
    checker.go
    resolver.go
)

GO_XTEST_SRCS(checker_test.go)

END()

RECURSE(
    gotest
    mocks
)
